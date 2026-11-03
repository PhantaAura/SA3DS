#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-player-death.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics \
 tests/test_player_death.c src/game/player_death.c src/game/player_death_player.c \
 src/game/player_damage.c src/game/player_damage_player.c src/game/stage_restart.c \
 src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c src/sa1/physics/*.c -lm -o "$task_tmp/test"
"$task_tmp/test"
