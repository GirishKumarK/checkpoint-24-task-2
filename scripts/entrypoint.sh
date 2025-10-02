#!/usr/bin/env bash
set -e
source /opt/ros/humble/setup.bash || true
if [ -f /root/ros2_ws/install/setup.bash ]; then
  source /root/ros2_ws/install/setup.bash
fi
exec "$@"
