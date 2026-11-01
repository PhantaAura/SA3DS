#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-half-turn.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
for contraction in fast off; do
  cc -std=c11 -O2 -ffp-contract="$contraction" -Wall -Wextra -Werror -Isrc/sa1/physics \
    tests/test_pc2004_half_turn.c src/sa1/physics/*.c -lm -o "$tmp/test"
  "$tmp/test"
done
