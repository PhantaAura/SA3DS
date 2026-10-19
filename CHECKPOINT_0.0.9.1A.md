# CHECKPOINT — SA3DS 0.0.9.1A

**Emerald Coast LandTable + Collision Decode**

Input code baseline: `0.0.9.0G — White Zone Validation Gate`.

Exact source authority used for this pass:

- `sonic.exe`
- 55,549,952 bytes
- SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`

Confirmed new roots:

- LandTable pointer table: `0x0097DA08`
- EC act 0 LandTable: `0x00E99CB8`
- EC act 1 LandTable: `0x01039E9C`
- EC act 2 LandTable: `0x010386B4`

Decoded source totals: 848 collision instances, 426 unique Basic models,
30,910 source points, 1,449 strip mesh sets, 46,903 triangles.

The new owner-side converter deterministically produces a pointer-free
`COLLISION` chunk in `.sa3dspak`.  Generated Sega-derived packages are excluded
from this code-only archive.

See:

- `docs/EMERALD_COAST_LANDTABLE_RECON_0.0.9.1A.md`
- `docs/EMERALD_COAST_COLLISION_FORMAT_0.0.9.1A.md`
- `EMERALD_COAST_ROOT_EVIDENCE_0.0.9.1A.json`
- `EMERALD_COAST_RECON_0.0.9.1A.json`
- `EMERALD_COAST_SYSTEM_REFERENCE_0.0.9.1A.json`
- `TEST_RESULTS_0.0.9.1A.txt`

Next: `0.0.9.1B` — stage visual/material/PVM decode, then SET/CAM bridge; do
not jump straight to claiming playable Emerald Coast.
