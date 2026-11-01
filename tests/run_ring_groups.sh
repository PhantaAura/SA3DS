#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ring-groups.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/make_ring_runtime_fixture.py "$tmp/rings.sa3dspak"
python3 tests/make_ring_groups_fixture.py "$tmp/rings.sa3dspak"
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak \
 tests/test_ring_groups.c src/game/ring_runtime.c src/game/ring_groups.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$tmp/rings.sa3dspak.groups"
for variant in count class mode children collision range nan endpoint; do "$tmp/test" "$tmp/rings.sa3dspak.groups.$variant" reject; done
