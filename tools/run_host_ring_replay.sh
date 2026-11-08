#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -lt 3 ] || [ "$#" -gt 5 ]; then echo "usage: $0 COLLISION ROUTE RING_PLACEMENTS [ACT [GROUPS_ONLY]]" >&2; exit 2; fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ring-replay.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror \
 -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
 tools/host_ring_replay.c src/game/ring_replay.c src/game/ring_runtime.c src/game/ring_groups.c src/game/player_view.c \
 src/game/emerald_coast_residency.c src/game/emerald_coast_route.c src/game/emerald_coast_collision.c src/game/pc2004_sphere_sweep.c \
 src/sa1/physics/*.c src/sa3c/sa3c_pose.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$@"
