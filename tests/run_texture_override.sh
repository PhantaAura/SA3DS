#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-override.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tests/test_dc_texture_override.py "$tmp"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c \
  tests/test_texture_override.c src/game/texture_override.c src/sa3c/sa3t_texture.c -o "$tmp/test"
"$tmp/test" "$tmp"
