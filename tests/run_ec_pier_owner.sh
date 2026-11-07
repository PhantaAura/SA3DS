#!/bin/sh
# Owner-data gate: PIER_PACKAGE and EC_COLLISION_PACKAGE are locally generated
# ec_pier / emerald_coast_collision .sa3dspak files (never committed).
set -eu
cd "$(dirname "$0")/.."
: "${PIER_PACKAGE:?set PIER_PACKAGE}" "${EC_COLLISION_PACKAGE:?set EC_COLLISION_PACKAGE}"
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-pier-owner.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
 tests/test_ec_pier_owner.c src/game/ec_pier.c src/game/emerald_coast_collision.c src/game/emerald_coast_residency.c \
 src/game/emerald_coast_route.c src/game/owned_task_list.c src/game/set_task_lifetime.c src/game/set_object_clip.c \
 src/game/set_preparation.c src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/sa1/physics/*.c -lm -o "$task_tmp/test"
"$task_tmp/test" "$PIER_PACKAGE" "$EC_COLLISION_PACKAGE"
