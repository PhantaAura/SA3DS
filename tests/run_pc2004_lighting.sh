#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
light_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-lighting.XXXXXX")"
trap 'rm -rf "$light_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_pc2004_lighting.c src/game/pc2004_lighting.c src/sa3dspak/sa3dspak.c -lm -o "$light_tmp/test"
"$light_tmp/test" "$@"
