#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
clip_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-set-clip.XXXXXX")"
trap 'rm -rf "$clip_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game tests/test_set_object_clip.c src/game/set_object_clip.c -lm -o "$clip_tmp/test"
"$clip_tmp/test"
