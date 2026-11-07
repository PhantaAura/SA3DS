#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-cam-trigger.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c tests/test_cam_trigger.c src/game/cam_trigger.c src/sa3c/sa3c_pose.c -lm -o "$task_tmp/test"
"$task_tmp/test"
