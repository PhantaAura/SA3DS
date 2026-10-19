# SA3DECOL v1 — pointer-free Emerald Coast collision payload

`tools/build_pc2004_emerald_coast_collision.py` converts the exact owner-owned
2004 `sonic.exe` into a source-neutral `COLLISION` chunk inside `.sa3dspak`.
The generated package is **not** included in the distributable checkpoint.

## Design rules

- no PC virtual addresses or pointers in the runtime collision payload
- deterministic ordering by exact source model identity during conversion
- one shared local-space model bank
- 848 source-neutral collision instances reference that model bank
- object rotations are converted from BAMS to radians
- local vertices remain float32
- triangle indices are u16; this exact EC source uses 30,910 vertices, safely
  below 65,536
- original surface/land flags and still-opaque per-entry fields are preserved
  losslessly for later semantic reconstruction

## Exact current output from the canonical owner executable

- acts: 3
- models: 426
- instances: 848
- vertices: 30,910
- triangle indices: 140,709
- triangles: 46,903
- collision chunk: 733,978 bytes
- `.sa3dspak`: 735,904 bytes

The generated `.sa3dspak` contains `METADATA` + `COLLISION` only.  Visual stage
geometry/material/texture chunks are deliberately deferred rather than guessed.

## Owner-side build

```sh
python3 tools/build_pc2004_emerald_coast_collision.py \
  /path/to/sonic.exe \
  /tmp/emerald_coast.sa3dspak \
  --report /tmp/emerald_coast.json

python3 tools/sa3dspak.py validate /tmp/emerald_coast.sa3dspak
```
