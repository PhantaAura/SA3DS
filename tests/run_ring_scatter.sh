#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ring-scatter.XXXXXX")"
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Isrc/game \
 tests/test_ring_scatter.c src/game/ring_scatter.c -lm -o "$task_tmp/test"
"$task_tmp/test"
python3 - "$task_tmp" <<'PY'
from pathlib import Path
import struct,sys
from tools import sa3dspak
out=Path(sys.argv[1]);b=bytearray(72);b[:8]=b'SA3DSCAT';struct.pack_into('<I',b,8,1);struct.pack_into('<f',b,16+20,5)
(out/'good.pak').write_bytes(sa3dspak.build([('SCATTER',b,0)]))
struct.pack_into('<I',b,12,1)
(out/'bad.pak').write_bytes(sa3dspak.build([('SCATTER',b,0)]))
PY
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game \
 tests/test_ring_scatter_assets.c src/game/ring_scatter_assets.c src/game/ccl_contact.c \
 src/game/enemy_damage.c src/sa3dspak/*.c -lm -o "$task_tmp/assets"
"$task_tmp/assets" "$task_tmp/good.pak" "$task_tmp/bad.pak"
