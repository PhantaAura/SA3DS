#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-set-lifetime.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_set_whale_lifetime.c src/game/set_task_lifetime.c src/game/set_object_clip.c src/game/whale_lifetime.c -lm -o "$task_tmp/test"
"$task_tmp/test"
