#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
checkpoint_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-checkpoint-task.XXXXXX")"
trap 'rm -rf "$checkpoint_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_checkpoint_task.c src/game/checkpoint_task.c -lm -o "$checkpoint_tmp/test"
"$checkpoint_tmp/test"
