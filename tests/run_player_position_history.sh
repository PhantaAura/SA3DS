#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
history_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-position-history.XXXXXX")"
trap 'rm -rf "$history_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_player_position_history.c src/game/player_position_history.c -o "$history_tmp/test"
"$history_tmp/test"
