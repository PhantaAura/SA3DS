#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-air-attack.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa1/physics \
 tests/test_pc2004_air_attack.c src/sa1/physics/*.c -lm -o "$tmp/test"
"$tmp/test"
