#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-animated-world.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c tests/test_sa3c_animated_world.c src/sa3c/sa3c_pose.c -lm -o "$tmp/test"
"$tmp/test"
