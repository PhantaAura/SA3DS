#!/usr/bin/env bash
# Read-only host diagnostic: no 3DS binary, no copyrighted content in package.
set -euo pipefail
cd "$(dirname "$0")"
if [[ $# -lt 1 || $# -gt 3 ]]; then
  echo 'Usage: bash Pass11A_MacAudit.sh /path/to/sonic.sa3c [pass10_pose_metrics.csv] [sonic.exe.c]' >&2
  exit 2
fi
sonic="$1"
if [[ ! -f "$sonic" ]]; then echo "Missing: $sonic" >&2; exit 2; fi
mkdir -p Pass11A_results
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
  tests/test_pass11_hierarchy.c -lm -o Pass11A_results/test_pass11_hierarchy
./Pass11A_results/test_pass11_hierarchy
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
  tools/pass11_pose_diag.c -lm -o Pass11A_results/pass11_pose_diag
./Pass11A_results/pass11_pose_diag "$sonic" 1 0 0 --nodes Pass11A_results/stand_action_nodes.csv > Pass11A_results/stand_action_frame0.csv
./Pass11A_results/pass11_pose_diag "$sonic" 1 0 1 --nodes Pass11A_results/stand_bind_nodes.csv > Pass11A_results/stand_bind_frame0.csv
./Pass11A_results/pass11_pose_diag "$sonic" 1 8 0 > Pass11A_results/stand_action_frame8.csv
python3 - <<'PY'
import csv
from pathlib import Path
p=Path('Pass11A_results')
files=['stand_action_frame0.csv','stand_bind_frame0.csv']
rows=[]
for f in files:
 with (p/f).open(newline='') as fd:rows.extend(csv.DictReader(fd))
with (p/'stand_frame0_pair.csv').open('w',newline='') as fd:
 w=csv.DictWriter(fd,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
PY
if [[ $# -ge 2 && -n "$2" ]]; then python3 tools/compare_pass10.py Pass11A_results/stand_frame0_pair.csv "$2"; fi
if [[ $# -ge 3 && -n "$3" ]]; then python3 tools/inspect_ghidra_export.py "$3"; fi
printf '\nPass11A audit complete. Results: %s/Pass11A_results\n' "$PWD"
