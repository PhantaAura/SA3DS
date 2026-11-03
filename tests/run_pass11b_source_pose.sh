#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
: "${CHRMODELS_PATH:?Set CHRMODELS_PATH to owner-owned original CHRMODELS_orig.dll}"
: "${SONIC_PVM_PATH:?Set SONIC_PVM_PATH to owner-owned SONIC.PVM}"
: "${SONIC_EXE_PATH:?Set SONIC_EXE_PATH to owner-owned original PC2004 sonic.exe}"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-pass11b.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 tools/verify_pc2004_pose_angles.py "$SONIC_EXE_PATH"
python3 tools/recon_pc2004_sonic_animdata.py \
  "$SONIC_EXE_PATH" --json "$tmp/pc2004_animdata.json" >/dev/null
cmp docs/PC2004_SONIC_ANIMDATA_REMAP.json "$tmp/pc2004_animdata.json"
echo 'PC2004 SonicAnimData executable reconstruction: PASS'
python3 tools/build_pc2004_sonic_sa3c.py \
  "$CHRMODELS_PATH" "$SONIC_PVM_PATH" "$tmp/sonic.sa3c" >/dev/null
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
  audits/pass11a/tools/pass11_pose_diag.c -lm -o "$tmp/pass11_pose_diag"
# Cover every source ACTION used by the first30 movement states and death86/87:
# idle/landing, jog/run, jump/SpinDash/Roll, spring/fall and uncurl.
# Derive from the independently reconstructed executable remap, not guessed
# action-slot names. Null movement entries would be a hard error here.
python3 - "$tmp/pc2004_animdata.json" > "$tmp/movement_actions.txt" <<'PY_ACTIONS'
import json, sys
mapping=json.load(open(sys.argv[1]))['dense_to_source_action_slot']
slots=sorted(set(mapping[:30]+[mapping[86],mapping[87]]))
if len(mapping)<30 or any(not isinstance(x,int) or x<0 for x in slots):
    raise SystemExit('invalid essential movement ACTION mapping')
for slot in slots:print(slot)
PY_ACTIONS
while IFS= read -r action; do
  python3 tools/audit_pass11b_source_pose.py \
    "$CHRMODELS_PATH" "$tmp/sonic.sa3c" "$tmp/pass11_pose_diag" --action "$action"
done < "$tmp/movement_actions.txt"

cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c \
  tests/transition_pose_dump.c -lm -o "$tmp/transition_pose_dump"
python3 tools/audit_pc2004_transition_pose.py \
  "$CHRMODELS_PATH" "$tmp/sonic.sa3c" "$tmp/transition_pose_dump"

cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c \
  tests/animated_world_dump.c -lm -o "$tmp/animated_world_dump"
python3 tools/audit_pc2004_animated_world.py \
  "$CHRMODELS_PATH" "$SONIC_EXE_PATH" "$tmp/sonic.sa3c" "$tmp/animated_world_dump"

# Exact executable damage selector/recoil/blink/ring-loss witnesses.
python3 tools/verify_pc2004_player_damage.py "$SONIC_EXE_PATH"

# Original scattered-ring dispatcher and source memory operands.
python3 tools/verify_pc2004_ring_scatter.py "$SONIC_EXE_PATH"

# Ordinary death task, source task order and action-stage restart/faders.
python3 tools/verify_pc2004_player_death.py "$SONIC_EXE_PATH"
