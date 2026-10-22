#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ec-contact.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c -Isrc/sa3dspak \
  tests/test_emerald_coast_collision.c src/game/emerald_coast_collision.c \
  src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/game/sonic_animation_sync.c \
  src/game/sonic_animation_runtime.c \
  src/sa1/physics/*.c -lm -o "$tmp/test"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c -Isrc/sa3dspak \
  tests/test_emerald_coast_contact_edges.c src/game/emerald_coast_collision.c \
  src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c \
  src/sa1/physics/*.c -lm -o "$tmp/edges"
if [ "$#" -eq 0 ]; then
  python3 tests/make_emerald_coast_collision_fixture.py "$tmp/fixture.sa3dspak"
  "$tmp/test" "$tmp/fixture.sa3dspak" synthetic
  "$tmp/edges" "$tmp/fixture.sa3dspak"
  for invalid in version model index nan overlap; do
    "$tmp/test" "$tmp/fixture.sa3dspak.$invalid" reject
  done
else
  "$tmp/test" "$@"
  if [ "$#" -eq 1 ]; then "$tmp/edges" "$1"; fi
fi
