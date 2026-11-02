#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ground-accel.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror \
  tests/test_pc2004_ground_accel.c src/sa1/physics/pc2004_ground_accel.c \
  src/sa1/physics/steam_player_parameters.c -lm -o "$tmp/test"
"$tmp/test"
