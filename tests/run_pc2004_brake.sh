#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-brake.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
 tests/test_pc2004_brake.c src/sa1/physics/*.c src/game/sonic_animation_runtime.c -lm -o "$tmp/test"
"$tmp/test"
