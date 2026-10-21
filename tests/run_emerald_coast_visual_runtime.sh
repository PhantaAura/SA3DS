#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="${TMPDIR:-/tmp}/sa3ds-ec-visual-runtime.$$"
fixture="$tmp.sa3dspak"
trap 'rm -f "$tmp" "$fixture"' EXIT HUP INT TERM
cc -std=c11 -Wall -Wextra -pedantic \
  -Isrc/game -Isrc/sa3dspak -Isrc/sa3c \
  tests/test_emerald_coast_visual_runtime.c \
  src/game/emerald_coast_visual.c src/sa3dspak/sa3dspak.c src/sa3c/sa3t_texture.c \
  -lm -o "$tmp"
if [ "$#" -eq 0 ]; then
  python3 tests/make_emerald_coast_visual_fixture.py "$fixture"
  "$tmp" "$fixture" synthetic
elif [ "$#" -eq 1 ]; then
  "$tmp" "$1"
else
  echo "usage: $0 [OWNER_GENERATED_ACT0.sa3dspak]" >&2
  exit 2
fi
