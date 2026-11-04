#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
whale_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-whale-trigger-assets.XXXXXX")"
trap 'rm -rf "$whale_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_whale_trigger_assets.c src/game/whale_trigger_assets.c src/game/whale_trigger.c src/sa3dspak/sa3dspak.c -lm -o "$whale_tmp/test"
if [ "$#" -ge 2 ]; then
 "$whale_tmp/test" "$1" "$2"
 shift 2
 for whale_bad in "$@"; do "$whale_tmp/test" "$whale_bad"; done
else
 "$whale_tmp/test" "$@"
fi
