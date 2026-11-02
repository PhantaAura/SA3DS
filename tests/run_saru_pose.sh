#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-saru-pose.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa3c \
 tests/test_saru_pose.c src/game/saru_pose.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_character.c -lm -o "$tmp/test"
"$tmp/test" "$@"
