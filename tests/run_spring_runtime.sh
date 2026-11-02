#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-spring-runtime.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics \
  tests/test_spring_runtime.c src/game/spring_runtime.c src/game/spring_launch.c \
  src/game/movement_objects.c src/sa3dspak/sa3dspak.c src/sa1/physics/pc2004_ground_accel.c src/sa1/physics/steam_player_parameters.c \
  -lm -o "$tmp/test"
"$tmp/test"
