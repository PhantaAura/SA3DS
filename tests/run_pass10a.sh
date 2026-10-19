#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tests/test_pass10a_source.py
bin="$(mktemp "${TMPDIR:-/tmp}/sa3ds-pose-metrics-XXXXXX")"
trap 'rm -f "$bin"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c -Isrc/game \
  src/sa3c/sa3c_render.c src/sa3c/sa3c_pose.c \
  src/game/sonic_pose_metrics.c tests/test_sonic_pose_metrics.c -lm -o "$bin"
"$bin"
