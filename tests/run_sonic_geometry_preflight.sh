#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_gpu_preflight.c src/sa3c/sa3t_texture.c \
  tests/test_sonic_geometry_preflight.c -lm -o "$TMP/geometry"
"$TMP/geometry"
python3 - <<'PY'
from pathlib import Path
s = Path('runtime_0_0_9_0F/source/main.c').read_text()
assert 'sa3ds_geometry_preflight(' in s and '&g_character, &g_geometry_report)' in s
assert 'sa3ds_geometry_result_name(geometry_result)' in s
assert s.index('sa3ds_geometry_preflight(') < s.index('sa3ds_sonic_gpu_init(')
print('native geometry loader integration: PASS (source contract; no 3DS compilation)')
PY
