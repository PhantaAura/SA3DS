#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-enemy-damage.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game \
 tests/test_enemy_damage.c src/game/enemy_damage.c src/game/game_score.c -lm -o "$tmp/test"
"$tmp/test"
