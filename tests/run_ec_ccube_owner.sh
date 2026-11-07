#!/bin/sh
# Owner-data gate: CCUBE_PACKAGE is a locally generated ec_ccube.sa3dspak (never committed).
set -eu
cd "$(dirname "$0")/.."
: "${CCUBE_PACKAGE:?set CCUBE_PACKAGE}"
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ccube-owner.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game -Isrc/sa3dspak tests/test_ec_ccube_owner.c src/game/ec_ccube.c \
 src/game/owned_task_list.c src/game/set_task_lifetime.c src/game/set_object_clip.c src/game/set_preparation.c src/sa3dspak/sa3dspak.c -lm -o "$task_tmp/test"
"$task_tmp/test" "$CCUBE_PACKAGE"
