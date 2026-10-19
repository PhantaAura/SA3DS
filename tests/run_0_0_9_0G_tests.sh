#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/sa3ds_0g_tests"
rm -rf "$B"; mkdir -p "$B"
trap 'rm -rf "$B"' EXIT INT TERM
common="src/sa1/physics/ground.c src/sa1/physics/surface_frame.c src/sa1/physics/physics_constants.c src/sa1/physics/movement.c src/sa1/physics/steam_input.c src/sa1/physics/steam_player_parameters.c src/sa1/physics/steam_air.c src/sa1/physics/steam_spindash_evidence.c src/sa1/physics/steam_ball.c src/sa1/physics/pc2004_motion.c src/sa1/physics/pc2004_collision_work.c src/sa1/physics/steam_surface_evidence.c src/sa1/physics/steam_surface.c src/sa1/physics/steam_flat.c"

echo '== White Zone 10-section geometry + selected reset =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  $common src/game/parity_lab.c tests/test_white_zone_parity_lab.c -lm -o "$B/lab"
"$B/lab"

echo '== HUD snapshot + FOLLOW/REFERENCE camera =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/ground.c src/sa1/physics/surface_frame.c src/sa1/physics/movement.c src/game/parity_lab.c src/game/parity_hud.c \
  tests/test_parity_hud.c -lm -o "$B/hud"
"$B/hud"

echo '== animation observer non-mutation stress =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/surface_frame.c src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  tests/test_animation_observer_h4.c -lm -o "$B/anim_stress"
"$B/anim_stress"

echo '== extended White Zone deterministic protocols =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  $common src/game/parity_lab.c src/game/parity_trace.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  tools/run_white_zone_protocol.c -lm -o "$B/protocol"
for s in flat_accel coast brake turn_low turn_high jump_tap jump_hold spindash roll_exit uphill downhill air_control; do
  "$B/protocol" "$s" 120 "$B/$s.csv" >/dev/null
  "$B/protocol" "$s" 120 "$B/${s}_repeat.csv" >/dev/null
  cmp "$B/$s.csv" "$B/${s}_repeat.csv"
done
python3 tests/test_white_zone_protocol_outputs.py "$B"

echo '== strict retail comparator =='
python3 tests/test_white_zone_trace_comparator.py
python3 tools/compare_white_zone_traces.py "$B/flat_accel.csv" "$B/flat_accel_repeat.csv" | grep 'STATUS: MATCHES MEASURED FIELDS' >/dev/null
python3 tools/compare_white_zone_traces.py "$B/flat_accel.csv" "$B/flat_accel_repeat.csv" --retail-live | grep 'STATUS: VERIFIED 1:1' >/dev/null

echo '== 0G runtime source contract =='
python3 tests/test_0_0_9_0G_runtime_source.py

echo '0.0.9.0G White Zone validation suite: PASS'
