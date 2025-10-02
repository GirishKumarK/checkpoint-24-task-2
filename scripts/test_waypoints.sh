#!/usr/bin/env bash
set -euo pipefail

# 1) Start sim (headless) in background
ros2 launch fastbot_gazebo main.launch.py headless:=true gui:=false >/tmp/gazebo_test.log 2>&1 &
SIM_PID=$!
sleep 8

# 2) Run tests
# Option A: ament/colcon tests for your package (recommended)
colcon test --packages-select fastbot_waypoints --return-code-on-test-failure \
  --event-handlers console_direct+ || TEST_FAILED=1

# 3) Stop sim
kill $SIM_PID || true
wait $SIM_PID || true

# 4) Bubble up failures to the caller (Jenkins)
if [ "${TEST_FAILED:-0}" = "1" ]; then
  echo "Tests failed."
  exit 1
fi
