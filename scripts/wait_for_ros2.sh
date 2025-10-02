#!/usr/bin/env bash
set -euo pipefail
for i in {1..30}; do
  if ros2 topic list >/dev/null 2>&1; then
    exit 0
  fi
  sleep 1
done
echo "ROS 2 not ready" >&2
exit 1
