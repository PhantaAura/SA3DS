#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
whale_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-whale-common.XXXXXX")"
trap 'rm -rf "$whale_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_whale_common.c src/game/whale_common.c -lm -o "$whale_tmp/test"
"$whale_tmp/test"
