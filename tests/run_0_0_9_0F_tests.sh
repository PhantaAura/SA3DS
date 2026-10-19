#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."

: "${CHRMODELS_PATH:=/mnt/data/CHRMODELS_clean.dll}"
: "${SONIC_PVM_PATH:=/mnt/data/_sonic_e_test.PVM}"
BUILD="${TMPDIR:-/tmp}/sa3ds_0f_tests"
rm -rf "$BUILD"; mkdir -p "$BUILD"
trap 'rm -rf "$BUILD"; rm -f runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3c runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3t runtime_0_0_9_0E/romfs/sa3ds/owner_asset_build_report.json' EXIT INT TERM

if [ "${SA3DS_SKIP_INHERITED:-0}" != "1" ]; then
  echo '== inherited 0.0.9.0E.1 owner-data gate =='
  CHRMODELS_PATH="$CHRMODELS_PATH" SONIC_PVM_PATH="$SONIC_PVM_PATH" sh tests/run_0_0_9_0E_tests.sh
else
  echo '== inherited gate skipped =='
fi

common="src/sa1/physics/ground.c src/sa1/physics/surface_frame.c src/sa1/physics/physics_constants.c src/sa1/physics/movement.c src/sa1/physics/steam_input.c src/sa1/physics/steam_player_parameters.c src/sa1/physics/steam_air.c src/sa1/physics/steam_spindash_evidence.c src/sa1/physics/steam_ball.c src/sa1/physics/pc2004_motion.c src/sa1/physics/pc2004_collision_work.c src/sa1/physics/steam_surface_evidence.c src/sa1/physics/steam_surface.c src/sa1/physics/steam_flat.c"

echo '== White Zone parity course contract =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  $common src/game/parity_lab.c tests/test_white_zone_parity_lab.c -lm \
  -o "$BUILD/test_white_zone_parity_lab"
"$BUILD/test_white_zone_parity_lab"

echo '== physics-synced real Sonic runtime selector =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/surface_frame.c src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  tests/test_sonic_animation_runtime.c -lm -o "$BUILD/test_sonic_animation_runtime"
"$BUILD/test_sonic_animation_runtime"

echo '== deterministic 1:1-check trace protocol =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  $common src/sa1/physics/physics_trace.c src/game/parity_lab.c \
  tools/run_white_zone_protocol.c -lm -o "$BUILD/white_zone_protocol"
for scenario in flat_accel coast brake jump_tap jump_hold spindash uphill downhill; do
  "$BUILD/white_zone_protocol" "$scenario" 120 "$BUILD/${scenario}_a.csv" >/dev/null
  "$BUILD/white_zone_protocol" "$scenario" 120 "$BUILD/${scenario}_b.csv" >/dev/null
  cmp "$BUILD/${scenario}_a.csv" "$BUILD/${scenario}_b.csv"
done
python3 tools/compare_traces.py "$BUILD/flat_accel_a.csv" "$BUILD/flat_accel_b.csv"

echo '== 3DS runtime source contract =='
python3 tests/test_0_0_9_0F_runtime_source.py

echo '0.0.9.0F White Zone parity foundation suite: PASS'
