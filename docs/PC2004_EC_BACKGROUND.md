# Original PC2004 Emerald Coast sky and horizon

This records original-game evidence and a bounded owner-only import path. It is
not a generated skybox, a replacement solid background or a claim of completed
renderer parity. Converted geometry/texture data, owner reports and source dumps
remain outside this code tree.

## Original evidence

The supported owner EXE SHA256 is
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The private decompilation and exact LLVM disassembly identify:

- Task initializer `0x4F7230` installs display callback `0x4F6DB0` at task+0x14.
- Config routine `0x4F6BB0` selects act×cliplevel scale, clip and fog rows.
- Background texture list `0x0103B3D4` resolves to `BG_BEACH.PVM`.
- Original Basic object roots `0x0103B270` and `0x0103B37C` are static identity
  leaves with original flags0x17. They contain the sky dome and horizon plane:
  57+4 raw points,3 meshes,3 materials. No invented cube/sphere is substituted.
- Draw code `0x4F6E2C..3D` translates by camera task+0x20/+0x28, with literal
  Y=0. Scale follows this translation through `0x788A50`; object transforms then
  follow. Camera altitude does not vertically anchor these models.
- Config table pointers are `0xE99E6C` scale, `0xE99E78` background clip,
  `0xE99E84` terrain clip, `0xE99E90` fog. Background far subtracts1000 before
  projection setup. `0x403180` maps negative source planes to projection state;
  the package exposes their positive distance representation.
- `cliplevel` is the original ini integer clamped0..2, default1; it is not a
  time-of-day state. All three rows for each of the two Sonic acts are packaged.
- `0x411B40` disables fog before drawing; through `0x787230` and `0x794980` it
  reaches D3D render state0x1C with0. `0x411AF0` restores stage fog afterward.
- Background clear RGB `0x003946` is selected for both Sonic acts. Act2's
  different state is outside Sonic's two-act background package.

The draw callback also saves render-control state, ORs bit2 and clears bit0x40,
then restores it. Full mapping of these control bits, Z/depth behavior,
lighting/material/raster precision and global render-order fidelity is not
established by this importer. Existing scene GPU code must not be described as
retail raster parity merely because it submits these original meshes.

`tools/verify_pc2004_ec_background.py OWNER_SONIC_EXE` verifies the exact source
callback/config/initializer ranges, archive binding and short instruction
witnesses. REA had no active target for this pass; retained private decompilation
plus exact original bytes/LLVM disassembly were used. No new Ghidra decompilation
or original-game runtime execution is claimed.

## Owner conversion

```sh
python3 tools/build_pc2004_ec_background.py \
  --exe '/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/sonic.exe' \
  --pvm '/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/system/BG_BEACH.PVM' \
  --output /private/tmp/emerald_coast_background.owner.sa3dspak \
  --report /private/tmp/emerald_coast_background.owner.json
```

Output paths inside this source checkout are rejected. The existing original
PC2004 Basic-model triangulator and PVM decoder are reused. A renderer-only
adapter labels the two source roots drawable for the existing EC visual format;
those labels are not original COL records and never supply collision.

The real local conversion produced269456 bytes, SHA256
`5492ae3caf4c6f2154638801e0384be23782e9ff53ae50b4620552d49b2bc2fe`:
2 models,3 meshes,2 instances,76 expanded vertices,318 indices/106 triangles,
3 textures. Raw model positions, source corner UVs and material values are
preserved through the established visual format. `BG_BEACH.PVM` source SHA256 is
`6ee04c83f990066cec8cad4763e3ee8176bf05ebb277ad9d337e996488d718a4`.
The texture bank contains original256×256 `sonic_sky1` and two8×8 source textures.
The exact generated file was accepted by the C runtime background reader.

## Reader and integration API

`emerald_coast_background.[ch]` opens an allocation-free bounded view containing:

- `visual`: standard `SA3DSECVisual`, suitable for existing full-act GPU batching.
- Six configuration rows for act0/1 and cliplevel0/1/2.
- Original scales, positive background/terrain clip distances, fog metadata,
  clear RGB and explicit camera-XZ/fog-off flags.

`sa3ds_ec_background_select` returns a row.
`sa3ds_ec_background_transform` returns
`Translate(camera.x,0,camera.z) * Scale(row.scale)` in source world units. It
rejects nonfinite camera inputs. Do not anchor Y to camera altitude or scale
camera translation. The GPU's mesh bounds remain local: disable background
culling, or transform the culling camera consistently with this model matrix.

Integrating code should draw before terrain with the background projection,
source matrix and fog disabled, then restore terrain projection/render state.
The API does not silently change the stage camera, gameplay or lighting. The
parent's native renderer integration/visual tests are tracked in the unified
WCA document; this source task did not edit existing GPU/main files.

The wrapper is a CRC-checked `.sa3dspak` with `METADATA`, `BACKGROUND` and
`VISUAL` chunks. `VISUAL` is a nested standard EC visual package. `BACKGROUND`
has a32-byte version1 header and six64-byte canonical act/clip records. File cap
2MiB; geometry caps8 models/instances,4096 vertices,16384 indices,16 materials;
texture caps16 entries/512KiB total,8..256 power-of-two dimensions. Bad CRC,
unknown/missing wrapper structure, malformed nested visual/config, nonfinite
values and invalid clip ordering fail atomically with a zero output view.

## Host verification

```sh
bash tests/run_ec_background.sh
python3 tools/verify_pc2004_ec_background.py OWNER_SONIC_EXE
```

Synthetic tests cover both acts/all clip rows, source transform order,
camera-altitude independence, nested CRC and malformed configuration rejection.
`tools/check_ec_background.c` is a read-only owner-file checker using the exact
runtime reader. Real conversion and that checker passed. Native build,
background submission, visual comparison and hardware acceptance are separate
checks and are not implied by these host results.

## Separate ocean/water animation findings

The original act tasks at `0x4F7300/7370` (act0) and
`0x4F73F0/7470/74E0` (act1) call `0x4F7250`, which changes model material texture
IDs from source record lists. Verified records include act0 lists
`0x01039EC0` (13 targets,base88,period2) and `0x01039F90`
(17 targets,base73,period5), and act1 lists `0x0103A0A0`
(12 targets,base62,period2) and `0x0103A160`
(14 targets,base47,period5). The tasks' source frame count is15. This is distinct
from the static BG_BEACH sky/horizon and from separate BEACH_SEA wave objects.
Call-phase/timing, target material remapping into imported deduplicated
materials, clipping and all act1 channels still require implementation and
verification. No static sky package is claimed to restore these animations.


## Native integration November 2

The native scene now loads the bounded external owner background package using
the existing CPU texture uploader. It draws both background material classes
before terrain with the source act/clip1 scale and background near/far planes,
anchored to camera X/Z and world altitude0. Its separate projection is restored
after drawing. The port background pass disables depth writes/testing, then
restores the scene depth state; full original lighting/material state remains
unresolved. This depth policy is a port integration choice, not a claim that all
original render states were recovered. No source fog is currently emulated.

Corrected format9 owner package269456B SHA256
329453fc5119b53f65cc614a4e3caff3fd22d52289dd3e2abf26acecade4fc52
supersedes the old incorrectly decoded texture package. Native combined build
315864B SHA256 b06f7ec7053651437da4dbe21bd0c6d67512883fe92d61f89c5688f3c9f7eeb4
compiled and rendered the original sky/horizon in the isolated preview through
the tick270 opening replay. Act1/clip transforms passed the host geometry gate;
act1 background raster and entire-course coverage remain unverified. GPU mock
checks exercise depth-off background submission and scene-state restoration.
