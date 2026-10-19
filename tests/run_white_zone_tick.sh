#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -Wall -Wextra -Werror -O2 -I src/game -I src/sa1/physics \
    tests/test_white_zone_tick.c src/game/white_zone_tick.c -o "$tmp/white_zone_tick"
"$tmp/white_zone_tick"
python3 tests/test_white_zone_tick_integration.py
