#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
edge_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-edge-equivalence.XXXXXX")"
trap 'rm -rf "$edge_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_ec_contact_edge_equivalence.c src/game/pc2004_sphere_sweep.c src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/sa1/physics/*.c -lm -o "$edge_tmp/test"
if [ "$#" -eq 0 ]; then
 python3 tests/make_emerald_coast_region_fixture.py "$edge_tmp/regions.sa3dspak"
 "$edge_tmp/test" "$edge_tmp/regions.sa3dspak"
else
 "$edge_tmp/test" "$@"
fi
