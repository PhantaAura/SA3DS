#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-move-objects.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/test_pc2004_movement_objects.py "$tmp/objects.sa3dspak"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_movement_objects.c src/game/movement_objects.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test" "$tmp/objects.sa3dspak"
for variant in count act slot type nan param spec duplicate truncated twice; do "$tmp/test" "$tmp/objects.sa3dspak.$variant" reject; done
