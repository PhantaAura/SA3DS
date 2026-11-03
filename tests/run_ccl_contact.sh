#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ccl-contact.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game \
 tests/test_ccl_contact.c src/game/ccl_contact.c src/game/enemy_damage.c -lm -o "$tmp/test"
"$tmp/test" "$@"
