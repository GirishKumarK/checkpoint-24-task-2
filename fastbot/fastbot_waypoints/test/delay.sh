#!/usr/bin/env bash
# Usage: delay.sh <seconds> <cmd...>
set -euo pipefail
delay="${1:-0}"
shift || true
sleep "$delay"
exec "$@"
