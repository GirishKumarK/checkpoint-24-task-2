#!/usr/bin/env bash
set -euo pipefail

# Example launch (change to your real FastBot sim launcher)
# Common pattern: fastbot_gazebo main.launch.py headless:=true gui:=false
ros2 launch fastbot_gazebo main.launch.py headless:=true gui:=false >/tmp/gazebo.log 2>&1 &
SIM_PID=$!

# Wait for sim to be alive (topics like /clock should appear)
echo "Waiting for /clock..."
timeout 60 bash -lc 'until ros2 topic list | grep -q "^/clock$"; do sleep 1; done'
echo "Gazebo is up (headless). PID=$SIM_PID"

# Keep foreground unless killed (Ctrl+C) — useful for manual runs
wait $SIM_PID
