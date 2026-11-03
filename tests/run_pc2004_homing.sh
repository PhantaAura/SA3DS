#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-homing.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa1/physics \
 tests/test_pc2004_homing.c src/sa1/physics/pc2004_homing.c -lm -o "$tmp/test"
"$tmp/test"
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa1/physics \
 tests/test_pc2004_homing_runtime.c src/sa1/physics/*.c -lm -o "$tmp/runtime"
"$tmp/runtime"
