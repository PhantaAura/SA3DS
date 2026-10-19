# SA3C v0 — Pointer-Free Character Staging Container

SA3C is an internal little-endian conversion format. It is generated from an
owner-supplied supported source and is **not distributed with SA3DS code-only
checkpoints** when it contains game-derived assets.

## Header

- magic: `SA3C`
- version: 0.0
- fixed header size: 272 bytes
- section count: 10
- flags
- total file size
- source CHRMODELS SHA-256
- 10 fixed-order section directory entries

Each directory entry is 20 bytes:

`fourcc, count, offset, stride, size`

All section starts are 16-byte aligned.

## Sections

| FourCC | Stride | Purpose |
|---|---:|---|
| NODE | 48 | pointer-free object hierarchy / bind transforms |
| MODL | 32 | model ranges into meshes/vertices/indices |
| MESH | 24 | material + converted triangle-list range |
| VRTX | 32 | position3, normal3, UV2 |
| INDX | 2 | global u16 vertex indices |
| MATL | 32 | source material fields + texture slot |
| TEXS | 24 | source PVM texture-slot metadata |
| ANIM | 24 | one record per source ACTION slot |
| CHAN | 24 | sparse per-node animation-channel ranges |
| KEYS | 16 | `u32 frame + float32 XYZ` |

Invalid u16 hierarchy/resource references use `0xFFFF`.

## Current v0 staging flags

Bit 0 means VRTX UV values preserve the source signed-16 UV values numerically
as float32. Final UV normalization is intentionally deferred until the exact
runtime sampling convention is certified.

## Source-to-neutral angle conversion

Node rotations and rotation keys are converted from the source integer angle
encoding to radians using one full turn = 65536 source units. Axis/order
application remains a renderer evidence gate; storage itself is pointer-free.

## Animation records

All 149 Sonic ACTION slots are represented. Null source action slots are kept as
empty ANIM records so source action numbers remain stable.

Each populated ANIM references a root NODE and a sparse CHAN range. Position,
rotation and scale keys are each stored as KEYS records. Source interpolation
values are preserved in ANIM; the exact interpolation functions are not guessed
by v0.

## Native validation

`src/sa3c/sa3c_character.c` validates:

- magic/version/header/directory layout;
- section sizes, alignment, ordering and bounds;
- node model/child/sibling references;
- model mesh/vertex/index ranges;
- mesh model/material/index ranges;
- every u16 index value;
- material texture-slot references;
- animation root/channel ranges;
- channel node/key ranges.
