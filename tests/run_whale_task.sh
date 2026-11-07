#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-whale-task.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1 -Isrc/sa3c -Isrc/sa3dspak -Isrc \
 tests/test_whale_task.c src/game/whale_task.c src/game/whale_actor.c src/game/whale_motion_phase.c src/game/whale_common.c \
 src/game/whale_tip.c src/game/whale_draw.c src/game/whale_ccl.c src/game/whale_lifetime.c src/game/ccl_contact.c src/game/enemy_damage.c \
 src/game/whale_trigger_runtime.c src/game/whale_trigger_assets.c src/game/whale_trigger.c src/game/owned_task_list.c \
 src/game/set_task_lifetime.c src/game/set_object_clip.c src/game/set_preparation.c src/sa3dspak/sa3dspak.c \
 -lm -o "$task_tmp/test"
"$task_tmp/test"
