#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
contact_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-checkpoint-contact.XXXXXX")"
trap 'rm -rf "$contact_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics \
 tests/test_checkpoint_contact.c src/game/checkpoint_contact.c src/game/ccl_contact.c src/game/enemy_damage.c -lm -o "$contact_tmp/test"
"$contact_tmp/test"
