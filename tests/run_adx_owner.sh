#!/bin/sh
# Owner-data gate: decodes the owner's ecoast1-3.adx (never committed).
set -eu
cd "$(dirname "$0")/.."
: "${BGM_DIR:?set BGM_DIR to the owner SoundData/bgm/wma folder}"
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-adx-owner.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_adx_owner.c src/game/adx.c -lm -o "$task_tmp/test"
"$task_tmp/test" "$BGM_DIR/ecoast1.adx" "$BGM_DIR/ecoast2.adx" "$BGM_DIR/ecoast3.adx"
