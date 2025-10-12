// fastbot_go_relative.cpp
#include <cmath>
#include <algorithm>
#include <string>
#include <array>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/pose2_d.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

class GoRelativeNode : public rclcpp::Node {
public:
  GoRelativeNode() : Node("fastbot_go_relative") {
    // --- tunables ---
    declare_parameter<double>("v_forward", 0.30);
    declare_parameter<double>("K_yaw_turn", 1.2);
    declare_parameter<double>("K_yaw_move", 1.0);
    declare_parameter<double>("w_max_turn", 1.0);
    declare_parameter<double>("w_max_move", 0.6);
    declare_parameter<double>("yaw_align_thresh_deg", 10.0);
    declare_parameter<double>("dist_tol", 0.05);
    declare_parameter<double>("final_yaw_tol_deg", 5.0);

    // --- frame correction + diagnostics ---
    declare_parameter<bool>("calibrate", true);
    declare_parameter<double>("odom_frame_correction_deg", 0.0);
    declare_parameter<bool>("publish_logical_odom", true);
    declare_parameter<bool>("debug", true);
    declare_parameter<double>("cal_v", 0.20);
    declare_parameter<double>("cal_time", 0.60);
    declare_parameter<bool>("accept_new_goals_during_motion", false);

    v_forward_         = get_parameter("v_forward").as_double();
    K_yaw_turn_        = get_parameter("K_yaw_turn").as_double();
    K_yaw_move_        = get_parameter("K_yaw_move").as_double();
    w_max_turn_        = get_parameter("w_max_turn").as_double();
    w_max_move_        = get_parameter("w_max_move").as_double();
    yaw_align_thresh_  = get_parameter("yaw_align_thresh_deg").as_double() * M_PI/180.0;
    dist_tol_          = get_parameter("dist_tol").as_double();
    final_yaw_tol_     = get_parameter("final_yaw_tol_deg").as_double() * M_PI/180.0;
    calibrate_         = get_parameter("calibrate").as_bool();
    publish_logical_odom_ = get_parameter("publish_logical_odom").as_bool();
    debug_             = get_parameter("debug").as_bool();
    cal_v_             = get_parameter("cal_v").as_double();
    cal_time_          = get_parameter("cal_time").as_double();
    accept_new_goals_during_motion_ = get_parameter("accept_new_goals_during_motion").as_bool();

    if (!calibrate_) setRotation(get_parameter("odom_frame_correction_deg").as_double() * M_PI/180.0);

    // I/O
    cmd_pub_ = create_publisher<geometry_msgs::msg::Twist>("/fastbot/cmd_vel", 10);
    odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
      "/fastbot/odom", 50, std::bind(&GoRelativeNode::odomCb, this, std::placeholders::_1));
    goal_sub_ = create_subscription<geometry_msgs::msg::Pose2D>(
      "/fastbot/goal_relative", 10, std::bind(&GoRelativeNode::goalCb, this, std::placeholders::_1));
    if (publish_logical_odom_) {
      odom_logical_pub_ = create_publisher<nav_msgs::msg::Odometry>("/fastbot/odom_logical", 10);
    }

    timer_ = create_wall_timer(std::chrono::milliseconds(40), std::bind(&GoRelativeNode::step, this));

    RCLCPP_INFO(get_logger(), "fastbot_go_relative started.");
    if (calibrate_) {
      RCLCPP_INFO(get_logger(), "Auto-calibrating frame rotation… (tiny forward nudge for %.2f s)", cal_time_);
    } else {
      RCLCPP_INFO(get_logger(), "Pose correction preset to %.1f deg", rot_corr_ * 180.0/M_PI);
    }
    RCLCPP_INFO(get_logger(), "Publish Pose2D to /fastbot/goal_relative (interpreted in PHYSICAL base frame)");
  }

private:
  // ------------ LOGICAL state for control (unchanged) ------------
  bool got_odom_{false}, have_goal_{false};
  double x_{0.0}, y_{0.0}, yaw_{0.0};
  double gx_{0.0}, gy_{0.0}, gyaw_{0.0};

  // ------------ REPORTED (raw) pose & twist ------------
  double xr_r_{0.0}, yr_r_{0.0}, yaw_r_{0.0};
  double vx_r_{0.0}, vy_r_{0.0};

  // ------------ Start pose & distance-only stop ------------
  double xr_start_{0.0}, yr_start_{0.0}, yaw_start_{0.0};
  double gx_b0_{0.0}, gy_b0_{0.0};
  double goal_dist_{0.0};  // NEW: commanded distance magnitude

  // ------------ params ------------
  double v_forward_, K_yaw_turn_, K_yaw_move_;
  double w_max_turn_, w_max_move_;
  double yaw_align_thresh_, dist_tol_, final_yaw_tol_;

  // rotation: reported -> logical
  double rot_corr_{0.0}, Rc_{1.0}, Rs_{0.0};
  bool publish_logical_odom_{true}, debug_{true};

  // auto-cal
  bool calibrate_{true};
  bool cal_running_{false}, cal_done_{false};
  double cal_v_{0.2}, cal_time_{0.6};
  rclcpp::Time cal_start_;
  double cal_vx_sum_{0.0}, cal_vy_sum_{0.0};
  int cal_samples_{0};
  bool accept_new_goals_during_motion_{false};

  // ros
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_logical_pub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<geometry_msgs::msg::Pose2D>::SharedPtr goal_sub_;
  rclcpp::TimerBase::SharedPtr timer_;

  // ------------ helpers ------------
  static double normAng(double a){ return std::atan2(std::sin(a), std::cos(a)); }
  inline void rotate2D(double xr, double yr, double &xl, double &yl) const {
    xl = Rc_*xr - Rs_*yr;
    yl = Rs_*xr + Rc_*yr;
  }
  void setRotation(double theta){
    rot_corr_ = theta; Rc_ = std::cos(theta); Rs_ = std::sin(theta);
    RCLCPP_INFO(get_logger(), "Using pose correction %.1f deg", rot_corr_ * 180.0/M_PI);
  }

  // ------------ callbacks ------------
  void odomCb(const nav_msgs::msg::Odometry::SharedPtr msg) {
    // (Your current swap preserved)
    xr_r_ = msg->pose.pose.position.y;
    yr_r_ = msg->pose.pose.position.x;

    const auto &q = msg->pose.pose.orientation;
    double r, p, yrep;
    tf2::Quaternion quat(q.x, q.y, q.z, q.w);
    tf2::Matrix3x3(quat).getRPY(r, p, yrep);
    yaw_r_ = yrep;

    vx_r_ = msg->twist.twist.linear.x;
    vy_r_ = msg->twist.twist.linear.y;

    rotate2D(xr_r_, yr_r_, x_, y_);
    yaw_ = normAng(yaw_r_ + rot_corr_);

    got_odom_ = true;

    if (publish_logical_odom_) {
      nav_msgs::msg::Odometry out = *msg;
      double vxl, vyl; rotate2D(vx_r_, vy_r_, vxl, vyl);
      tf2::Quaternion q_out; q_out.setRPY(0,0,yaw_);
      out.pose.pose.position.x = x_;
      out.pose.pose.position.y = y_;
      out.pose.pose.orientation.x = q_out.x();
      out.pose.pose.orientation.y = q_out.y();
      out.pose.pose.orientation.z = q_out.z();
      out.pose.pose.orientation.w = q_out.w();
      out.twist.twist.linear.x = vxl;
      out.twist.twist.linear.y = vyl;
      out.header.frame_id = "fastbot_odom_logical";
      out.child_frame_id  = "fastbot_base_logical";
      odom_logical_pub_->publish(out);
    }
  }

  void goalCb(const geometry_msgs::msg::Pose2D::SharedPtr g) {
    if (!got_odom_) { RCLCPP_WARN(get_logger(), "No odom yet; ignoring goal."); return; }
    if (have_goal_ && !accept_new_goals_during_motion_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *this->get_clock(), 1000,
        "Ignoring new goal while executing current one.");
      return;
    }

    // Freeze START pose and the commanded vector in START body frame
    xr_start_ = xr_r_;
    yr_start_ = yr_r_;
    yaw_start_ = yaw_r_;
    gx_b0_ = g->x;
    gy_b0_ = g->y;
    goal_dist_ = std::hypot(gx_b0_, gy_b0_);  // <— key line

    // existing logs/world goal (not used for stopping)
    const double c_r = std::cos(yaw_r_), s_r = std::sin(yaw_r_);
    const double gx_r = xr_r_ + c_r * g->x - s_r * g->y;
    const double gy_r = yr_r_ + s_r * g->x + c_r * g->y;
    const double gyaw_r = normAng(yaw_r_ + g->theta);
    rotate2D(gx_r, gy_r, gx_, gy_);
    gyaw_ = normAng(gyaw_r + rot_corr_);

    have_goal_ = true;
    if (debug_) {
      RCLCPP_INFO(get_logger(),
        "New goal: rel(B0)=(%.3f,%.3f,%.1f°) -> world_logical=(%.3f,%.3f,%.1f°), dist0=%.3f",
        g->x, g->y, g->theta * 180.0/M_PI, gx_, gy_, gyaw_*180.0/M_PI, goal_dist_);
    }
  }

  // ------------ main loop ------------
  void step() {
    if (!got_odom_) return;

    // ----- auto-calibration (unchanged) -----
    if (calibrate_ && !cal_done_) {
      geometry_msgs::msg::Twist cmd;
      if (!cal_running_) {
        cal_running_ = true;
        cal_start_ = now();
        cal_vx_sum_ = cal_vy_sum_ = 0.0;
        cal_samples_ = 0;
        RCLCPP_INFO(get_logger(), "Calibration: nudging forward v=%.2f m/s for %.2f s…", cal_v_, cal_time_);
      }
      cmd.linear.x = cal_v_;
      cmd_pub_->publish(cmd);

      cal_vx_sum_ += vx_r_;
      cal_vy_sum_ += vy_r_;
      cal_samples_++;

      const double t = (now() - cal_start_).seconds();
      if (t >= cal_time_) {
        cmd.linear.x = 0.0; cmd.angular.z = 0.0; cmd_pub_->publish(cmd);
        const double ax = (cal_samples_>0) ? cal_vx_sum_/cal_samples_ : 0.0;
        const double ay = (cal_samples_>0) ? cal_vy_sum_/cal_samples_ : 0.0;

        struct Candidate { double deg; double score; };
        std::array<Candidate,4> cands;
        cands[0] = {   0.0,  ax };
        cands[1] = {  90.0, -ay };
        cands[2] = { -90.0,  ay };
        cands[3] = { 180.0, -ax };

        auto best = cands[0];
        for (size_t i=1;i<cands.size();++i) if (cands[i].score > best.score) best = cands[i];

        setRotation(best.deg * M_PI/180.0);
        cal_done_ = true;
        RCLCPP_INFO(get_logger(), "Calibration: avg reported twist (vx=%.3f, vy=%.3f) -> chose %.0f° rotation",
                    ax, ay, best.deg);
      }
      return;
    }

    geometry_msgs::msg::Twist cmd;
    if (!have_goal_) { cmd_pub_->publish(cmd); return; }

    // ===== Distance-only stop, measured in START body frame (B0) =====
    // World displacement from START:
    const double dxr = xr_r_ - xr_start_;
    const double dyr = yr_r_ - yr_start_;
    // Rotate into START body frame B0: d_b0 = R(-yaw_start_) * d_r
    const double c0 = std::cos(yaw_start_), s0 = std::sin(yaw_start_);
    const double x_b0 =  c0 * dxr + s0 * dyr;
    const double y_b0 = -s0 * dxr + c0 * dyr;
    const double s_travel = std::hypot(x_b0, y_b0);

    // Stop as soon as traveled distance >= commanded distance (with tolerance)
    if (s_travel + dist_tol_ >= goal_dist_) {
      have_goal_ = false;
      if (debug_) {
        RCLCPP_INFO(get_logger(),
          "Distance goal reached: traveled=%.3f, target=%.3f (B0). Stopping.",
          s_travel, goal_dist_);
      }
      geometry_msgs::msg::Twist stop; // zeros
      cmd_pub_->publish(stop);
      return;
    }
    // ===== End distance-only stop =====

    // Use your existing controller to move toward the (logical) goal
    const double dx = gx_ - x_;
    const double dy = gy_ - y_;
    const double dist = std::hypot(dx, dy);

    if (dist > dist_tol_) {
      const double des = std::atan2(dy, dx);
      const double eyaw = normAng(des - yaw_);

      if (std::fabs(eyaw) > yaw_align_thresh_) {
        cmd.linear.x = 0.0;
        cmd.angular.z = std::clamp(K_yaw_turn_ * eyaw, -w_max_turn_, w_max_turn_);
      } else {
        cmd.linear.x = v_forward_;
        cmd.angular.z = std::clamp(K_yaw_move_ * eyaw, -w_max_move_, w_max_move_);
      }
    } else {
      cmd.linear.x = 0.0;
      cmd.angular.z = 0.0;
    }

    if (debug_) {
      RCLCPP_INFO_THROTTLE(get_logger(), *this->get_clock(), 500,
        "travel(B0)=%.3f / %.3f  now_b0=(%.3f,%.3f)  cmd(v=%.2f,w=%.2f)",
        s_travel, goal_dist_, x_b0, y_b0, cmd.linear.x, cmd.angular.z);
    }
    cmd_pub_->publish(cmd);
  }
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GoRelativeNode>());
  rclcpp::shutdown();
  return 0;
}
