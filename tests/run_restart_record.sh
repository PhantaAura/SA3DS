#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
record_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-restart-record.XXXXXX")"
trap 'rm -rf "$record_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_restart_record.c src/game/restart_record.c -o "$record_tmp/test"
"$record_tmp/test"
