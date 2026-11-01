#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -ne 4 ]; then echo "usage: $0 COLLISION ACT0 ACT1 ROUTE" >&2; exit 2; fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-owner-regions.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_emerald_coast_owner_residency.c src/game/emerald_coast_residency.c src/game/emerald_coast_route.c \
 src/game/emerald_coast_collision.c src/game/emerald_coast_visual.c src/sa3dspak/sa3dspak.c \
 src/sa3c/sa3c_pose.c src/sa3c/sa3t_texture.c src/sa1/physics/*.c -lm -o "$tmp/test"
"$tmp/test" "$@"
