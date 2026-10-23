#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
: "${CHRMODELS_PATH:?Set CHRMODELS_PATH to owner-owned original CHRMODELS_orig.dll}"
: "${SONIC_PVM_PATH:?Set SONIC_PVM_PATH to owner-owned SONIC.PVM}"
: "${SONIC_EXE_PATH:?Set SONIC_EXE_PATH to owner-owned original PC2004 sonic.exe}"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-pass11b.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tools/recon_pc2004_sonic_animdata.py \
  "$SONIC_EXE_PATH" --json "$tmp/pc2004_animdata.json" >/dev/null
cmp docs/PC2004_SONIC_ANIMDATA_REMAP.json "$tmp/pc2004_animdata.json"
echo 'PC2004 SonicAnimData executable reconstruction: PASS'
python3 tools/build_pc2004_sonic_sa3c.py \
  "$CHRMODELS_PATH" "$SONIC_PVM_PATH" "$tmp/sonic.sa3c" >/dev/null
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
  audits/pass11a/tools/pass11_pose_diag.c -lm -o "$tmp/pass11_pose_diag"
for action in 1 22 122 123; do
  python3 tools/audit_pass11b_source_pose.py \
    "$CHRMODELS_PATH" "$tmp/sonic.sa3c" "$tmp/pass11_pose_diag" --action "$action"
done
