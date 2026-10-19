# Emerald Coast LandTable reconstruction — 0.0.9.1A

## Authority

Primary authority is the exact supported 2004 PC executable:

- file: `sonic.exe`
- size: `55,549,952` bytes
- SHA-256: `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`

The decoder rejects every other executable fingerprint.  No Sega payload is
included in this checkpoint.

## Stage-1 loader/root proof

The executable contains `_Stg01.rel` at `0x007DE4D0`.  The Emerald Coast stage
selection case at `0x00422B5E` pushes that exact string.  In this PC build, the
called routine at `0x0051F680` is a one-byte `RET`, while the real stage data is
already reachable in the executable image.  This is direct evidence that the
old REL name survives as a marker even though this exact executable is using
statically linked stage data at runtime.

The generic LandTable access path at `0x0043A260` computes:

`slot = second_argument + first_argument * 8`

and then loads:

`[slot * 4 + 0x0097DA08]`

The three non-null entries at level id 1 / act ids 0..2 are therefore the
Emerald Coast LandTables.

## Exact Emerald Coast roots

| Act | LandTable | Collision list | Entries | Texture list | PVM stem |
|---:|---:|---:|---:|---:|---|
| 0 | `0x00E99CB8` | `0x00E95938` | 480 | `0x00F812AC` | `BEACH01` |
| 1 | `0x01039E9C` | `0x010386D8` | 169 | `0x00EF553C` | `BEACH02` |
| 2 | `0x010386B4` | `0x01036AB8` | 199 | `0x00E9A4CC` | `BEACH03` |

For all three acts:

- LandTable radius field = exactly `3000.0f`
- auxiliary count = `0`
- load flags = `0x000C`
- initial load/ref field = `0`
- `collision_list + collision_count * 0x24 == LandTable`

That last invariant is especially strong structural evidence: each collision
array ends exactly where its LandTable begins.

## Proven structure accesses

The checked loader/runtime path proves:

### LandTable — 0x1C bytes

| Offset | Evidence-safe meaning |
|---:|---|
| `+0x00` | signed 16-bit collision-entry count |
| `+0x02` | signed 16-bit auxiliary count; exact semantics not promoted yet |
| `+0x04` | 16-bit load/behavior flags tested by runtime code |
| `+0x06` | 16-bit load/ref state modified by runtime code |
| `+0x08` | finite positive radius/clip scalar; EC value `3000.0f` |
| `+0x0C` | collision-entry array pointer |
| `+0x10` | auxiliary-list pointer |
| `+0x14` | auxiliary/hook pointer |
| `+0x18` | texture-list pointer |

The runtime reads `word [LandTable+0]`, then `[LandTable+0x0C]`, and advances
the collision array in exact `0x24`-byte strides.

### Collision entry — 0x24 bytes

| Offset | Evidence-safe meaning |
|---:|---|
| `+0x00..0x0F` | four finite floats, structurally the entry bounding sphere |
| `+0x10` | preserved opaque field |
| `+0x14` | preserved opaque field |
| `+0x18` | NJS_OBJECT pointer — directly consumed by retail code |
| `+0x1C` | preserved opaque field |
| `+0x20` | surface/land flags — directly masked by retail code |

The retail setup path uses `collision+0x18`, reads `collision+0x20`, and advances
by exactly `0x24` bytes.

## Object/model validation

All 848 Emerald Coast collision entries in this exact source validate as:

- non-null object pointer
- one flat `0x34` NJS_OBJECT-compatible node
- no child/sibling on the collision root
- non-null `0x2C` Basic-model pointer
- finite object transforms and model bounds
- valid point, normal, mesh-set, and material ranges

Across all three acts:

- collision instances: **848**
- unique objects: **848**
- unique Basic models: **426**
- source points: **30,910**
- mesh sets: **1,449**
- material records referenced by models: **1,554**
- decoded triangles: **46,903**
- all 1,449 checked mesh sets use Basic strip primitive class `0xC000`

Per act:

| Act | Instances | Unique models | Points | Mesh sets | Triangles |
|---:|---:|---:|---:|---:|---:|
| 0 | 480 | 183 | 13,323 | 578 | 19,598 |
| 1 | 169 | 142 | 10,515 | 557 | 16,450 |
| 2 | 199 | 101 | 7,072 | 314 | 10,855 |

## What is not claimed yet

0.0.9.1A does **not** claim:

- retail polygon/contact solver parity on the imported geometry
- stage material/texture rendering parity
- complete PVM conversion for `BEACH01/02/03`
- SET object semantics
- CAM semantics
- water/background/object runtime
- Old 3DS stage rendering or performance

Those remain later 0.0.9.1 / 0.0.9.2 gates.
