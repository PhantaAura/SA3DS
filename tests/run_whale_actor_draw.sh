#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
whale_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-whale-actor.XXXXXX")"
trap 'rm -rf "$whale_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c -Isrc/sa3dspak tests/test_whale_actor_draw.c src/game/whale_actor.c src/game/whale_draw.c src/game/whale_motion_phase.c src/game/whale_common.c src/game/whale_tip.c src/sa3dspak/sa3dspak.c -lm -o "$whale_tmp/test"
"$whale_tmp/test"
