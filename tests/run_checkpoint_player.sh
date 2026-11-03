#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
player_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-checkpoint-player.XXXXXX")"
trap 'rm -rf "$player_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_checkpoint_player.c src/game/checkpoint_player.c src/game/checkpoint_task.c src/game/restart_record.c src/game/course_clock.c \
 src/sa1/physics/*.c -lm -o "$player_tmp/test"
"$player_tmp/test"
