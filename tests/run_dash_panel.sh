#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-panel.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics \
 tests/test_dash_panel.c src/sa1/physics/*.c src/game/stage.c \
 src/game/dash_panel.c src/game/spring_runtime.c src/game/spring_launch.c \
 src/game/movement_objects.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test"
