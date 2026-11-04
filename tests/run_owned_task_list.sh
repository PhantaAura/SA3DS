#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-owned-tasks.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_owned_task_list.c src/game/owned_task_list.c -o "$task_tmp/test"
"$task_tmp/test"
