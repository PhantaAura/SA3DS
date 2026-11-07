#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-box-push.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_ccl_box_push.c src/game/ccl_box_push.c src/game/checkpoint_contact.c src/game/ccl_contact.c src/game/enemy_damage.c -lm -o "$task_tmp/test"
"$task_tmp/test"
