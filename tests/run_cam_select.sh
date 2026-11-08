#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-camsel.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_cam_select.c src/game/cam_select.c src/game/cam_trigger.c -lm -o "$task_tmp/test"
"$task_tmp/test"
