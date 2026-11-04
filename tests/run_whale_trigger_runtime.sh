#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-whale-trigger-runtime.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_whale_trigger_runtime.c src/game/whale_trigger_runtime.c src/game/owned_task_list.c src/game/set_task_lifetime.c src/game/set_object_clip.c src/game/whale_trigger_assets.c src/game/whale_trigger.c src/sa3dspak/sa3dspak.c -lm -o "$task_tmp/test"
"$task_tmp/test"
