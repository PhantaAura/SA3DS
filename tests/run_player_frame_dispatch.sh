#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-frame-dispatch.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/sa1/physics \
  tests/test_player_frame_dispatch.c src/sa1/physics/*.c -lm -o "$tmp/test"
"$tmp/test"
