#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-rings.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/make_ring_runtime_fixture.py "$tmp/rings.sa3dspak"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_ring_runtime.c src/game/ring_runtime.c src/game/ring_groups.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$tmp/rings.sa3dspak"
"$tmp/test" "$tmp/rings.sa3dspak.capacity" capacity
for variant in count act index type nan radius clip duplicate; do "$tmp/test" "$tmp/rings.sa3dspak.$variant" reject; done
