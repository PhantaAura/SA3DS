#!/bin/sh
set -eu
if [ "$#" -ne 2 ]; then
  echo "usage: $0 /path/to/CHRMODELS.DLL /path/to/SONIC.PVM" >&2
  exit 2
fi
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
python3 "$ROOT/tools/prepare_3ds_owner_assets.py" "$1" "$2" --runtime "$ROOT/runtime_0_0_9_0F"
make -C "$ROOT/runtime_0_0_9_0F"
echo
echo "Built: $ROOT/runtime_0_0_9_0F/sa3ds_0_0_9_0F_white_zone_parity.3dsx"
