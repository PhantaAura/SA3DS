#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-player-view.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/platform/3ds -Isrc/sa3c -Isrc/sa3dspak \
  tests/test_player_view.c src/game/player_view.c src/platform/3ds/input_adapter_math.c \
  src/game/emerald_coast_collision.c src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c \
  src/sa1/physics/*.c -lm -o "$tmp/test"
"$tmp/test" "$@"
python3 tests/test_player_view_integration.py
