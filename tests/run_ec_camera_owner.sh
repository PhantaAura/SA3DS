#!/bin/sh
# Owner-data gate: CAMERA_PACKAGE is a locally generated ec_camera.sa3dspak (never committed).
set -eu
cd "$(dirname "$0")/.."
: "${CAMERA_PACKAGE:?set CAMERA_PACKAGE}"
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-camera-owner.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_ec_camera_owner.c src/game/cam_trigger.c src/sa3dspak/sa3dspak.c -lm -o "$task_tmp/test"
"$task_tmp/test" "$CAMERA_PACKAGE"
