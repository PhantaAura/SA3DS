#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/surface_frame.c src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  tests/test_animation_tick_stability.c -lm -o "$BUILD/tick"
"$BUILD/tick"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa1/physics -Isrc/game \
  src/sa1/physics/surface_frame.c src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_sync.c src/game/sonic_animation_runtime.c \
  tests/test_sonic_animation_runtime.c -lm -o "$BUILD/inherited"
"$BUILD/inherited"
python3 - <<'PY'
from pathlib import Path
main=Path('runtime_0_0_9_0F/source/main.c').read_text()
reset=main.split('static void reset_selected_zone(void) {',1)[1].split('static void cycle_zone(',1)[0]
assert 'memset(&g_anim_sync' in reset
assert 'memset(&g_anim_runtime' in reset
assert 'g_observation=sa3ds_sonic_animation_observe(' in reset
assert 'g_selection=sa3ds_sonic_animation_select(' in reset
print('native zone-reset pose refresh source contract: PASS')
PY
