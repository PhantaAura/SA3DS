#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ec-regions.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/make_emerald_coast_region_fixture.py "$tmp/regions.sa3dspak"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
  tests/test_emerald_coast_regions.c src/game/emerald_coast_collision.c src/game/pc2004_sphere_sweep.c \
  src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/sa1/physics/*.c -lm -o "$tmp/test"
"$tmp/test" "$tmp/regions.sa3dspak"
