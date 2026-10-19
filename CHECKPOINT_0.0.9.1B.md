# CHECKPOINT — SA3DS 0.0.9.1B

**Emerald Coast Visual / Material / PVM Decode**

Baseline: `0.0.9.1A — Emerald Coast LandTable + Collision Decode`.

## Confirmed in this checkpoint

- Acts 0/1/2 now convert into deterministic pointer-free visual geometry.
- Basic model points, normals, mesh streams, materials and per-corner UVs are
  decoded from the exact canonical PC2004 executable.
- Stage UV normalization is source `signed16 / 255.0`; out-of-range tiled UVs
  are preserved.
- Act material texture IDs are valid local indices into BEACH01/02/03 PVM.
- Six Emerald Coast PVM archives parse under a strict generic PC2004 parser.
- Pixel formats 0/1/2 and data format 9 are supported for the audited files.
- Format-9 largest-mip ordering is **base first**.  This is cross-archive proven
  and fixes the older SONIC.PVM base-mip extraction rule.
- Owner-side per-act packages self-validate as `.sa3dspak` with metadata,
  geometry, materials and neutral SA3T textures.

## Evidence status

- Model/material/UV/PVM structural decode: **CONFIRMED** for exact audited
  sources.
- Material flag bit semantics: **UNKNOWN / preserved raw**.
- Final PICA200 texture format/residency strategy: **NOT YET IMPLEMENTED**.
- SET/CAM/object/event semantics: **NEXT PASSES**.

No Sega executable, DLL, PVM, generated stage package, texture pixels, or
compiled 3DS binary is included in this source checkpoint.
