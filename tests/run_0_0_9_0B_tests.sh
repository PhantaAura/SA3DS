#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

echo "== inherited 0.0.9.0A + frozen 0.0.8.8.9 suite =="
sh tests/run_0_0_9_0A_tests.sh

build_dir="${TMPDIR:-/tmp}/sa3ds_anim_0_0_9_0B"
mkdir -p "$build_dir"
common="src/sa1/physics/ground.c src/sa1/physics/surface_frame.c src/sa1/physics/physics_constants.c src/sa1/physics/movement.c src/sa1/physics/steam_input.c src/sa1/physics/steam_player_parameters.c src/sa1/physics/steam_air.c src/sa1/physics/steam_spindash_evidence.c src/sa1/physics/steam_ball.c src/sa1/physics/pc2004_motion.c src/sa1/physics/pc2004_collision_work.c src/sa1/physics/steam_surface_evidence.c src/sa1/physics/steam_surface.c src/sa1/physics/steam_flat.c src/game/stage.c"

echo "== 0.0.9.0B physics-synced presentation tests =="
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/surface_frame.c src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_sync.c tests/test_sonic_animation_sync.c -lm \
  -o "$build_dir/test_sonic_animation_sync"
"$build_dir/test_sonic_animation_sync"

cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  $common src/game/sonic_animation_sync.c tests/test_sonic_animation_sync_runtime.c -lm \
  -o "$build_dir/test_sonic_animation_sync_runtime"
"$build_dir/test_sonic_animation_sync_runtime"

python3 tests/test_animation_intent_trace.py
python3 tests/test_pc2004_sonic_action_accesses.py

echo "0.0.9.0B suite: PASS"
