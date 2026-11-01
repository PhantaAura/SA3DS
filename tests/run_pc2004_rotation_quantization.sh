#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-rotation-quantization.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa3c tests/test_pc2004_rotation_quantization.c src/sa3c/sa3c_pose.c -lm -o "$tmp/test"
"$tmp/test"
