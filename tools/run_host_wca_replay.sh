#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -ne 2 ]; then echo 'usage: run_host_wca_replay.sh owner_collision.sa3dspak output.csv' >&2; exit 2; fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-wca-replay.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game -Isrc/sa1/physics -Isrc/sa3c -Isrc/sa3dspak \
  tools/host_wca_replay.c src/game/wca_replay.c src/game/player_view.c \
  src/game/emerald_coast_collision.c src/game/pc2004_sphere_sweep.c src/game/parity_trace.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  src/sa3dspak/sa3dspak.c src/sa3c/sa3c_pose.c src/sa1/physics/*.c -lm -o "$tmp/replay"
"$tmp/replay" "$@"
