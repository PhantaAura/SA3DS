#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-edge-precision.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
for contraction in off fast; do
  cc -std=c11 -O2 -ffp-contract="$contraction" -Wall -Wextra -Werror \
    -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
    tests/test_ec_edge_precision.c src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c \
    src/sa1/physics/*.c -lm -o "$tmp/test"
  "$tmp/test"
done
