#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
menu_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-gameover.XXXXXX")"
trap 'rm -rf "$menu_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_game_over.c src/game/game_over.c -o "$menu_tmp/test"
"$menu_tmp/test"
