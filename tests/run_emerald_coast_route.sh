#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ec-route.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/make_emerald_coast_route_fixture.py "$tmp/route.sa3dspak"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak \
  tests/test_emerald_coast_route.c src/game/emerald_coast_route.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$tmp/route.sa3dspak"
"$tmp/test" "$tmp/route.sa3dspak-goal" goal
"$tmp/test" "$tmp/route.sa3dspak-oriented" oriented
for bad in "$tmp"/route.sa3dspak.*; do "$tmp/test" "$bad" reject; done
