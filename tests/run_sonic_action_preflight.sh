#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c -Isrc/game \
  src/sa3c/sa3c_pose.c src/game/sonic_action_preflight.c \
  tests/test_sonic_action_preflight.c -lm -o "$TMP/preflight"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c -Isrc/game \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c \
  src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_runtime.c src/game/sonic_action_preflight.c \
  tools/check_sonic_actions.c -lm -o "$TMP/check_sonic_actions"
python3 tests/make_synthetic_sonic_sa3c.py "$TMP/synthetic.sa3c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c \
  tests/test_sonic_transition_pose.c -lm -o "$TMP/test_transition_pose"
"$TMP/test_transition_pose" "$TMP/synthetic.sa3c"
"$TMP/check_sonic_actions" "$TMP/synthetic.sa3c"
python3 tests/make_synthetic_sonic_sa3c.py --bad-key "$TMP/corrupt.sa3c"
if "$TMP/check_sonic_actions" "$TMP/corrupt.sa3c" > "$TMP/bad.out" 2>&1; then
  echo 'FAIL: hidden corrupt key was not detected' >&2; exit 1
fi
grep -q 'NONFINITE.*key=1' "$TMP/bad.out"
"$TMP/preflight"
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c -Isrc/game \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c \
  src/sa1/physics/steam_player_parameters.c \
  src/game/sonic_animation_runtime.c src/game/sonic_action_preflight.c \
  tools/check_sonic_actions.c -lm -o "$TMP/check_sonic_actions"
python3 tests/make_synthetic_sonic_sa3c.py "$TMP/synthetic.sa3c"
"$TMP/check_sonic_actions" "$TMP/synthetic.sa3c"
python3 tests/make_synthetic_sonic_sa3c.py --bad-key "$TMP/corrupt.sa3c"
if "$TMP/check_sonic_actions" "$TMP/corrupt.sa3c" > "$TMP/bad.out" 2>&1; then
  echo 'FAIL: hidden corrupt key was not detected' >&2; exit 1
fi
grep -q 'NONFINITE.*key=1' "$TMP/bad.out"
python3 - <<'PY'
from pathlib import Path
s=Path('runtime_0_0_9_0F/source/main.c').read_text()
assert '#include "../../src/game/sonic_action_preflight.h"' in s
assert 'sa3ds_sonic_animation_binding_count()' in s
assert 'sa3ds_sonic_animation_binding_at(' in s
assert 'sa3ds_action_preflight(' in s
assert 'sa3ds_action_preflight_result_name(' in s
print('native Sonic ACTION preflight source integration: PASS (not 3DS compiled)')
PY
