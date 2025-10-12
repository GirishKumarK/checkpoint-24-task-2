#include <gtest/gtest.h>
#include <chrono>
#include <cmath>
#include <memory>
#include <cstdlib>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose2_d.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

// -------------------- Config (overridable via env) --------------------
static double kGoalX = 0.25;          // FASTBOT_TARGET_DX
static double kGoalY = 0.00;          // FASTBOT_TARGET_DY
static double kDistTol = 0.05;        // FASTBOT_DIST_TOL
static double kStopVThresh = 1e-3;    // FASTBOT_STOP_V
static double kStopWThresh = 1e-3;    // FASTBOT_STOP_W
static int    kStopCount = 5;         // FASTBOT_STOP_COUNT (consecutive msgs)
static int    kTimeoutSec = 60;       // FASTBOT_TIMEOUT

static double envd(const char* n, double d){
  if (const char* v = std::getenv(n)) { try { return std::stod(v); } catch (...) {} }
  return d;
}
static int envi(const char* n, int d){
  if (const char* v = std::getenv(n)) { try { return std::stoi(v); } catch (...) {} }
  return d;
}

class FastbotDistanceStopTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    // env overrides
    kGoalX        = envd("FASTBOT_TARGET_DX", kGoalX);
    kGoalY        = envd("FASTBOT_TARGET_DY", kGoalY);
    kDistTol      = envd("FASTBOT_DIST_TOL", kDistTol);
    kStopVThresh  = envd("FASTBOT_STOP_V", kStopVThresh);
    kStopWThresh  = envd("FASTBOT_STOP_W", kStopWThresh);
    kStopCount    = envi("FASTBOT_STOP_COUNT", kStopCount);
    kTimeoutSec   = envi("FASTBOT_TIMEOUT", kTimeoutSec);

    int argc = 0; char **argv = nullptr;
    rclcpp::init(argc, argv);
    node_ = rclcpp::Node::make_shared("fastbot_distance_stop_test");

    // Prefer logical odom
    odom_sub_ = node_->create_subscription<nav_msgs::msg::Odometry>(
      "/fastbot/odom_logical", 10,
      [](const nav_msgs::msg::Odometry::SharedPtr msg){ onOdom(msg, /*logical*/true); });

    // Also listen to raw odom as fallback
    odom_raw_sub_ = node_->create_subscription<nav_msgs::msg::Odometry>(
      "/fastbot/odom", 10,
      [](const nav_msgs::msg::Odometry::SharedPtr msg){ onOdom(msg, /*logical*/false); });

    goal_pub_ = node_->create_publisher<geometry_msgs::msg::Pose2D>("/fastbot/goal_relative", 10);

    // Wait for any odom
    const auto t0 = std::chrono::steady_clock::now();
    while (rclcpp::ok() && !has_any_odom_) {
      rclcpp::spin_some(node_);
      if (std::chrono::steady_clock::now() - t0 > std::chrono::seconds(15)) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(has_any_odom_) << "No odom received on /fastbot/odom_logical or /fastbot/odom";

    // Freeze start pose from the preferred stream
    use_logical_ = has_logical_odom_;
    if (use_logical_) {
      x0_ = last_x_logical_; y0_ = last_y_logical_; yaw0_ = last_yaw_logical_;
    } else {
      x0_ = last_x_raw_;     y0_ = last_y_raw_;     yaw0_ = last_yaw_raw_;
    }

    // Publish goal (few times to be safe)
    geometry_msgs::msg::Pose2D g;
    g.x = kGoalX; g.y = kGoalY; g.theta = 0.0;
    for (int i=0;i<5;i++) {
      goal_pub_->publish(g);
      rclcpp::spin_some(node_);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Main wait loop: stop when traveled >= target - tol AND velocity ~ 0 for kStopCount frames
    const double target_dist = std::hypot(kGoalX, kGoalY);
    int stop_ok_count = 0;

    const auto tstart = std::chrono::steady_clock::now();
    while (rclcpp::ok()) {
      rclcpp::spin_some(node_);

      // Current pose & twist from chosen stream
      double x,y,yaw,vx,vy,wz;
      if (use_logical_) {
        x = last_x_logical_; y = last_y_logical_; yaw = last_yaw_logical_;
        vx = last_vx_logical_; vy = last_vy_logical_; wz = last_wz_logical_;
      } else {
        x = last_x_raw_; y = last_y_raw_; yaw = last_yaw_raw_;
        vx = last_vx_raw_; vy = last_vy_raw_; wz = last_wz_raw_;
      }

      // Displacement in start body frame B0 (same math as controller)
      const double dx = x - x0_;
      const double dy = y - y0_;
      const double c0 = std::cos(yaw0_), s0 = std::sin(yaw0_);
      const double xb0 =  c0*dx + s0*dy;
      const double yb0 = -s0*dx + c0*dy;
      const double traveled = std::hypot(xb0, yb0);

      // Speed magnitude (use linear in body frame and yaw rate)
      const double vmag = std::hypot(vx, vy);

      if (traveled + kDistTol >= target_dist && vmag <= kStopVThresh && std::fabs(wz) <= kStopWThresh) {
        stop_ok_count++;
        if (stop_ok_count >= kStopCount) {
          // record finals
          traveled_final_ = traveled;
          vmag_final_ = vmag;
          wz_final_ = wz;
          break;
        }
      } else {
        stop_ok_count = 0; // reset window
      }

      if (std::chrono::steady_clock::now() - tstart > std::chrono::seconds(kTimeoutSec)) {
        FAIL() << "Timeout waiting for distance stop";
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }

  static void TearDownTestSuite() {
    goal_pub_.reset();
    odom_sub_.reset();
    odom_raw_sub_.reset();
    node_.reset();
    rclcpp::shutdown();
  }

  // --------- Odom callback (logical/raw) ----------
  static void onOdom(const nav_msgs::msg::Odometry::SharedPtr msg, bool logical) {
    const auto &p = msg->pose.pose.position;
    const auto &q = msg->pose.pose.orientation;
    tf2::Quaternion quat(q.x, q.y, q.z, q.w);
    double r,pitch,y; tf2::Matrix3x3(quat).getRPY(r,pitch,y);

    const auto &tw = msg->twist.twist;

    if (logical) {
      last_x_logical_ = p.x; last_y_logical_ = p.y; last_yaw_logical_ = y;
      last_vx_logical_ = tw.linear.x; last_vy_logical_ = tw.linear.y; last_wz_logical_ = tw.angular.z;
      has_logical_odom_ = true;
    } else {
      last_x_raw_ = p.x; last_y_raw_ = p.y; last_yaw_raw_ = y;
      last_vx_raw_ = tw.linear.x; last_vy_raw_ = tw.linear.y; last_wz_raw_ = tw.angular.z;
    }
    has_any_odom_ = true;
  }

  // ----------------- Shared state -----------------
  static rclcpp::Node::SharedPtr node_;
  static rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;      // logical
  static rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_raw_sub_;  // raw
  static rclcpp::Publisher<geometry_msgs::msg::Pose2D>::SharedPtr goal_pub_;

  static bool has_any_odom_, has_logical_odom_, use_logical_;
  static double x0_, y0_, yaw0_;

  // last logical
  static double last_x_logical_, last_y_logical_, last_yaw_logical_;
  static double last_vx_logical_, last_vy_logical_, last_wz_logical_;
  // last raw
  static double last_x_raw_, last_y_raw_, last_yaw_raw_;
  static double last_vx_raw_, last_vy_raw_, last_wz_raw_;

  static double traveled_final_, vmag_final_, wz_final_;
};

// ---- static definitions
rclcpp::Node::SharedPtr FastbotDistanceStopTest::node_;
rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr FastbotDistanceStopTest::odom_sub_;
rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr FastbotDistanceStopTest::odom_raw_sub_;
rclcpp::Publisher<geometry_msgs::msg::Pose2D>::SharedPtr FastbotDistanceStopTest::goal_pub_;
bool FastbotDistanceStopTest::has_any_odom_ = false;
bool FastbotDistanceStopTest::has_logical_odom_ = false;
bool FastbotDistanceStopTest::use_logical_ = false;
double FastbotDistanceStopTest::x0_ = 0, FastbotDistanceStopTest::y0_ = 0, FastbotDistanceStopTest::yaw0_ = 0;

double FastbotDistanceStopTest::last_x_logical_ = 0, FastbotDistanceStopTest::last_y_logical_ = 0, FastbotDistanceStopTest::last_yaw_logical_ = 0;
double FastbotDistanceStopTest::last_vx_logical_ = 0, FastbotDistanceStopTest::last_vy_logical_ = 0, FastbotDistanceStopTest::last_wz_logical_ = 0;

double FastbotDistanceStopTest::last_x_raw_ = 0, FastbotDistanceStopTest::last_y_raw_ = 0, FastbotDistanceStopTest::last_yaw_raw_ = 0;
double FastbotDistanceStopTest::last_vx_raw_ = 0, FastbotDistanceStopTest::last_vy_raw_ = 0, FastbotDistanceStopTest::last_wz_raw_ = 0;

double FastbotDistanceStopTest::traveled_final_ = 0;
double FastbotDistanceStopTest::vmag_final_ = 0;
double FastbotDistanceStopTest::wz_final_ = 0;

// -------------------- TESTS --------------------

// 1) It reaches (or exceeds) the commanded path length within tolerance
TEST_F(FastbotDistanceStopTest, DistanceReachedAndStopped) {
  // Just sanity: controller should have reached within tolerance; velocity should be ~0
  EXPECT_GE(traveled_final_ + kDistTol, std::hypot(kGoalX, kGoalY))
      << "Traveled distance did not reach target within tolerance";
  EXPECT_LE(vmag_final_, kStopVThresh) << "Linear speed not near zero at stop";
  EXPECT_LE(std::fabs(wz_final_), kStopWThresh) << "Angular speed not near zero at stop";
}