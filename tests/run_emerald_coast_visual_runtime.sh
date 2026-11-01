#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="${TMPDIR:-/tmp}/sa3ds-ec-visual-runtime.$$"
fixture="$tmp.sa3dspak"
trap 'rm -f "$tmp" "$fixture" "$fixture".set "$fixture".cam "$fixture".both "$fixture".duplicate "$fixture".unknown' EXIT HUP INT TERM
cc -std=c11 -Wall -Wextra -pedantic \
  -Isrc/game -Isrc/sa3dspak -Isrc/sa3c \
  tests/test_emerald_coast_visual_runtime.c \
  src/game/emerald_coast_visual.c src/sa3dspak/sa3dspak.c src/sa3c/sa3t_texture.c \
  -lm -o "$tmp"
if [ "$#" -eq 0 ]; then
  python3 tests/make_emerald_coast_visual_fixture.py "$fixture"
  "$tmp" "$fixture" synthetic
  python3 - "$fixture" <<'PYFIX'
from pathlib import Path
import sys
sys.path.insert(0,'.')
from tools import sa3dspak
p=Path(sys.argv[1]);base=[(c['type'],c['data'],c['flags']) for c in sa3dspak.parse(p.read_bytes())['chunks']]
for suffix,extra in [('set',[('SET',b'opaque bank',0)]),('cam',[('CAMERA',b'opaque bank',0)]),('both',[('SET',b'opaque bank',0),('CAMERA',b'opaque bank',0)]),('duplicate',[base[0]]),('unknown',[('AUDIO',b'unknown visual class',0)])]:
    Path(str(p)+'.'+suffix).write_bytes(sa3dspak.build(base+extra))
PYFIX
  for variant in set cam both; do "$tmp" "$fixture.$variant" synthetic; done
  for variant in duplicate unknown; do "$tmp" "$fixture.$variant" reject; done
elif [ "$#" -eq 1 ]; then
  "$tmp" "$1"
else
  echo "usage: $0 [OWNER_GENERATED_ACT0.sa3dspak]" >&2
  exit 2
fi
