#!/bin/sh
set -eu
build_dir="${TMPDIR:-/tmp}/sa3ds_host_tests"
mkdir -p "$build_dir"
common="src/sa1/physics/ground.c src/sa1/physics/surface_frame.c src/sa1/physics/physics_constants.c src/sa1/physics/movement.c src/sa1/physics/steam_input.c src/sa1/physics/steam_player_parameters.c src/sa1/physics/steam_air.c src/sa1/physics/steam_spindash_evidence.c src/sa1/physics/steam_ball.c src/sa1/physics/pc2004_motion.c src/sa1/physics/pc2004_collision_work.c src/sa1/physics/steam_surface_evidence.c src/sa1/physics/steam_surface.c src/sa1/physics/steam_flat.c src/game/stage.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_physics.c -lm -o "$build_dir/test_physics"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_steam_flat.c -lm -o "$build_dir/test_steam_flat"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_steam_air.c -lm -o "$build_dir/test_steam_air"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_steam_ball.c -lm -o "$build_dir/test_steam_ball"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_surface_frame.c -lm -o "$build_dir/test_surface_frame"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_surface_evidence.c -lm -o "$build_dir/test_surface_evidence"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_pc2004_motion.c -lm -o "$build_dir/test_pc2004_motion"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_pc2004_collision_work.c -lm -o "$build_dir/test_pc2004_collision_work"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common tests/test_surface_runtime.c -lm -o "$build_dir/test_surface_runtime"
"$build_dir/test_surface_frame"
"$build_dir/test_surface_evidence"
"$build_dir/test_pc2004_motion"
"$build_dir/test_pc2004_collision_work"
"$build_dir/test_surface_runtime"
"$build_dir/test_steam_flat"
"$build_dir/test_steam_air"
"$build_dir/test_steam_ball"
"$build_dir/test_physics"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game $common src/sa1/physics/physics_trace.c tests/physics_runner.c -lm -o "$build_dir/physics_runner"
"$build_dir/physics_runner" accel_flat 8 "$build_dir/determinism_a.csv"
"$build_dir/physics_runner" accel_flat 8 "$build_dir/determinism_b.csv"
cmp "$build_dir/determinism_a.csv" "$build_dir/determinism_b.csv"
python3 tools/compare_traces.py "$build_dir/determinism_a.csv" "$build_dir/determinism_b.csv"
awk -F, 'BEGIN{OFS=","} NR==2{$19=-30} {print}' "$build_dir/determinism_a.csv" > "$build_dir/divergent.csv"
if python3 tools/compare_traces.py "$build_dir/determinism_a.csv" "$build_dir/divergent.csv" >/dev/null; then
  echo "divergent trace unexpectedly matched" >&2; exit 1
fi
echo "trace determinism test: PASS"
for scenario in accel_flat coast_from_speed brake_from_speed reverse_from_speed jump_tap jump_hold air_control gentle_uphill gentle_downhill steep_uphill bank_transition high_speed_turn low_speed_turn fall_reset spindash_action spindash_shortcut loop_roll loop_slow_wall_support loop_fast_wall_stick; do
  "$build_dir/physics_runner" "$scenario" 240 "$build_dir/$scenario.csv" >/dev/null
done
python3 tests/test_scenarios.py "$build_dir"
python3 tools/sa3dspak.py pack "$build_dir/synthetic.sa3dspak" --source-profile synthetic-original-v1 \
  --chunk GEOMETRY=tests/data/synthetic_geometry.txt \
  --chunk COLLISION=tests/data/synthetic_collision.txt
python3 tools/sa3dspak.py validate "$build_dir/synthetic.sa3dspak"
python3 tools/sa3dspak.py inspect "$build_dir/synthetic.sa3dspak" >/dev/null
python3 tools/sa3dspak.py extract "$build_dir/synthetic.sa3dspak" "$build_dir/extracted"
cmp tests/data/synthetic_geometry.txt "$build_dir/extracted/001_GEOMETRY.bin"
cmp tests/data/synthetic_collision.txt "$build_dir/extracted/002_COLLISION.bin"
cp "$build_dir/synthetic.sa3dspak" "$build_dir/corrupt.sa3dspak"
printf 'X' | dd of="$build_dir/corrupt.sa3dspak" bs=1 seek=96 conv=notrunc 2>/dev/null
if python3 tools/sa3dspak.py validate "$build_dir/corrupt.sa3dspak" 2>/dev/null; then
  echo "corrupt package unexpectedly validated" >&2; exit 1
fi
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3dspak src/sa3dspak/sa3dspak.c tests/test_sa3dspak.c -o "$build_dir/test_sa3dspak"
"$build_dir/test_sa3dspak" "$build_dir/synthetic.sa3dspak" "$build_dir/corrupt.sa3dspak"
python3 tests/test_sa3dspak.py
