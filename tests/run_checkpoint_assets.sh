#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
assets_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-checkpoint-assets.XXXXXX")"
trap 'rm -rf "$assets_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_checkpoint_assets.c src/game/checkpoint_assets.c src/game/movement_objects.c \
 src/game/emerald_coast_visual.c src/sa3c/sa3t_texture.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_character.c src/sa3dspak/sa3dspak.c -lm -o "$assets_tmp/test"
if [ "$#" -gt 0 ]; then "$assets_tmp/test" "$@"; else
 python3 tests/make_emerald_coast_visual_fixture.py "$assets_tmp/visual.sa3dspak"
 python3 tests/make_checkpoint_assets_fixture.py "$assets_tmp/visual.sa3dspak" "$assets_tmp/checkpoint.sa3dspak"
 "$assets_tmp/test" "$assets_tmp/checkpoint.sa3dspak" "$assets_tmp/checkpoint.reserved.sa3dspak" "$assets_tmp/checkpoint.cci.sa3dspak" "$assets_tmp/checkpoint.version.sa3dspak"
fi
