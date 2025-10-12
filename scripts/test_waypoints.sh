#!/usr/bin/env bash
#set -euo pipefail

# --- Env (ROS 2) ---
source /opt/ros/humble/setup.bash
[ -f /root/ros2_ws/install/setup.bash ] && source /root/ros2_ws/install/setup.bash || true

echo "ROS_DISTRO=${ROS_DISTRO:-unknown}"
echo "ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-not set}"

# --- Ensure display (for headless fallback) ---
#if [[ -z "${DISPLAY:-}" ]]; then
#  export DISPLAY=:99
#  Xvfb :99 -screen 0 1280x800x24 &
#  sleep 1
#fi

#SIM_PID=""


# Launch the sim (GUI if DISPLAY is real; headless if Xvfb)
#exec roslaunch tortoisebot_gazebo tortoisebot_playground.launch
#ros2 launch fastbot_gazebo one_fastbot_room.launch.py

# --- Wait for existing Gazebo (Classic) or launch locally (headless) ---
#echo "Waiting for Gazebo (node '/gazebo') up to 10s..."
#if ! timeout 10 bash -lc 'until ros2 node list 2>/dev/null | grep -qx "/gazebo"; do sleep 1; done'; then
#  echo "No external Gazebo found; launching headless sim locally..."
  # Adjust package/launch file and arg names to your setup:
#  ros2 launch fastbot_gazebo one_fastbot_room.launch.py gui:=false >/tmp/gazebo_tests.log 2>&1 &
#  SIM_PID=$!
#  echo "Waiting for Gazebo node from local launch (up to 120s)..."
#  timeout 120 bash -lc 'until ros2 node list 2>/dev/null | grep -qx "/gazebo"; do sleep 1; done'
#fi
#echo "Gazebo is up."

echo "[debug] current topics:"
ros2 topic list

ros2 launch fastbot_waypoints test_waypoints_action.launch.py