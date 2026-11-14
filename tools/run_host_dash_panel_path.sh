#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -ne 2 ]; then
    echo 'usage: run_host_dash_panel_path.sh owner_collision.sa3dspak owner_movement.sa3dspak' >&2
    exit 2
fi
tmp=$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-panel-path.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror \
  -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
  tools/host_dash_panel_path.c src/game/emerald_coast_collision.c \
  src/game/emerald_coast_residency.c src/game/emerald_coast_route.c \
  src/game/pc2004_sphere_sweep.c src/game/dash_panel.c src/game/spring_player.c \
  src/game/spring_runtime.c src/game/spring_launch.c src/game/movement_objects.c \
  src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/sa1/physics/*.c -lm -o "$tmp/replay"
"$tmp/replay" "$@"
