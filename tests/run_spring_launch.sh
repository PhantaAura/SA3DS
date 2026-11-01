#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-spring-launch.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror tests/test_spring_launch.c src/game/spring_launch.c src/game/movement_objects.c src/sa3dspak/sa3dspak.c -lm -o "$tmp/test"
"$tmp/test"
