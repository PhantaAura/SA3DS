#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-saru-actor.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak \
 tests/test_saru_actor.c src/game/saru_actor.c src/game/saru.c src/game/movement_objects.c \
 src/game/ring_groups.c src/game/ring_runtime.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$@"
