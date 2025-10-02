#!/usr/bin/env bash
set -euo pipefail

# ROS env
source /opt/ros/humble/setup.bash
source /ws/install/setup.bash || source /ws/install/local_setup.bash || true

# Headless X server for Gazebo
Xvfb :99 -screen 0 1280x1024x24 >/tmp/xvfb.log 2>&1 &
XVFB_PID=$!
export DISPLAY=:99

cleanup() {
  pkill -INT -f "ros2 launch"  >/dev/null 2>&1 || true
  pkill -INT -f "gz"           >/dev/null 2>&1 || true
  kill $XVFB_PID               >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Quick health check
/ci/wait_for_ros2.sh

# Run package tests (invokes pytest in ros2_ci_tests)
colcon test --event-handlers console_direct+ --packages-select ros2_ci_tests
colcon test-result --verbose

echo "[CI][ROS2] Tests finished successfully."
