#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-background.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/make_ec_background_fixture.py "$tmp"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_ec_background.c src/game/emerald_coast_background.c src/game/emerald_coast_visual.c \
 src/sa3c/sa3t_texture.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$tmp"
