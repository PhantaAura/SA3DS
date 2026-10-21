# Emerald Coast visual/material package — 0.0.9.1B

`0.0.9.1B` extends the exact `0.0.9.1A` LandTable/collision decode into a
pointer-free visual representation for Emerald Coast acts 0, 1, and 2.
Generated packages contain Sega-derived geometry and texture pixels and are
**owner-side build products only**.  They are never included in the code-only
checkpoint archive.

## Owner input

- canonical PC2004 `sonic.exe`
- owner `system` folder or ZIP containing `BEACH01.PVM`, `BEACH02.PVM`, and
  `BEACH03.PVM`

The generic PVM auditor also validates `OBJ_BEACH.PVM`, `BEACH_SEA.PVM`, and
`BG_BEACH.PVM` for future object/sea/background work.

## Generated act package

Each `emerald_coast_actN.sa3dspak` contains:

- `METADATA` — source hashes, counts, UV/material evidence and limitations;
- `GEOMETRY` — internal `SA3DVIS` v1 pointer-free geometry/instances;
- `MATERIALS` — internal `SA3DMATL` v1 exact material fields;
- `TEXTURES` — neutral `SA3T` RGBA8 base textures decoded from the act PVM.

## SA3DVIS v1

The visual geometry stores:

- source-neutral model records;
- mesh records;
- per-act instance transforms;
- vertex position + normal + UV (`float32`, 32-byte vertex);
- 16-bit triangle-list indices;
- raw source primitive/type-material fields;
- raw object and LandTable surface flags on instances.

The converter deterministically expands Basic strips/quads into triangle lists
using the same conventions already pinned by the Sonic character pipeline.

### Runtime visibility subset

PC2004 land draw-list builder `FUN_0043a900`, exact `sonic.exe` SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`,
tests COL+0x20 as signed at `0x0043AA5B` and skips non-negative entries.
The runtime therefore requires LandTable flag bit 31 for visual residency;
the separate collision reader still uses its solid subset. Keeping all nearby
entries visually rendered incorrectly exposed collision-only meshes. Other
visibility-mask/frustum conditions are not claimed reconstructed here.
`tools/verify_pc2004_stage_scale.py` checks the original branch and source
coordinates against local owner packages without exporting game payloads.

### UV semantics

Current PC2004 Basic stage UVs are signed 16-bit per-corner values.  SA Tools
semantics already established the divisor as exactly `255.0`; the neutral
stage vertex stores `source_uv / 255.0` as float32.  Stage UVs intentionally
extend outside `[0,1]` for tiling.

Observed raw ranges:

| Act | U min | U max | V min | V max |
|---|---:|---:|---:|---:|
| 0 | -49 | 10200 | -5100 | 276 |
| 1 | -688 | 15197 | -5100 | 767 |
| 2 | -384 | 15300 | -5100 | 296 |

Meshes with a null UV pointer are explicitly flagged as having no UVs rather
than being assigned invented coordinates.

## SA3DMATL v1

The exact 20-byte Basic material fields are preserved:

1. diffuse `u32`
2. specular `u32`
3. exponent `float32`
4. texture ID `u32`
5. flags `u32`

For every current textured Emerald Coast model, the mesh material field is a
valid local zero-based material index.  One act-2 collision model has zero
materials; its material field is zero and is represented with an invalid
neutral material index rather than guessing a render material.

Every material texture ID is range-checked against the corresponding act PVM.
Material flag bit meanings are deliberately preserved raw pending direct
runtime/code evidence.

## Exact current counts

| Act | Models | Meshes | Instances | Neutral vertices | Indices | Triangles | Dedup materials | No-UV meshes | PVM textures |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 183 | 578 | 480 | 22,237 | 58,794 | 19,598 | 237 | 61 | 103 |
| 1 | 142 | 557 | 169 | 16,460 | 49,350 | 16,450 | 120 | 25 | 91 |
| 2 | 101 | 314 | 199 | 10,418 | 32,565 | 10,855 | 122 | 14 | 89 |

All three per-act vertex banks remain below the 65,535 limit required by the
current 16-bit index representation.

## Important Old 3DS note

The generated neutral packages are correctness/import artifacts, not final
memory budgets.  Their embedded RGBA8 `SA3T` payloads are intentionally easy
to validate but larger than the original 16-bpp PVR data.  The future runtime
must avoid loading every texture archive simultaneously and should perform
PICA200-oriented texture conversion offline.

## Native Checkpoint A integration (2026-10-20)

The 0G native runtime now accepts the owner-generated act-0 package from
`sdmc:/3ds/SA3DS/assets/emerald_coast_act0.sa3dspak` through boot-menu `R`.
It validates every package region and cross-reference before allocation, then
keeps a 500-unit opening subset resident: 45 instances, 48 material batches,
5,694 triangles, 17,082 expanded static vertices, and 25 of 103 textures.
Instance transforms use the existing Ninja object matrix rules; UVs remain
unclamped and texture addressing remains repeat. The stage textures use the
same CPU-swizzled upload path as Sonic.

Azahar 2126.0 Old 3DS/Vulkan visibly rendered the authentic connected geometry
and water. This checkpoint does not claim material-flag semantics, transparent
sorting, original lighting/camera, collision, a retail spawn, 60 FPS, or
physical-hardware success. White Zone remains separately accessible on `A`.
