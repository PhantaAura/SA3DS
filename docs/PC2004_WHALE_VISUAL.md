# WCA65 — original whale Basic visual and seam-preserving native update path

Owner-only builder now packages the original Basic model, five materials, four
referenced OBJ_BEACH textures, both WCA64 shape banks and a pointer-free source
vertex map. Display502210 selects420FC0; the level1 texture table at90EB68
resolves that context to OBJ_BEACH. Texture slots27..30 are compacted locally;
source material diffuse/specular/exponent/flags remain intact. No owner payload
is embedded in the executable or committed to this repository.

The optional source-identity collector in the established geometry converter
records(model address, source index) whenever a UV-split vertex is emitted.
It does not infer identity from matching positions. The whale has281 render
vertices and1380 ordered triangle corners, with25 source vertices using multiple
UVs. The map preserves both sampled position and sampled normal at every seam,
while leaving original UVs unchanged. Default converter output is byte-identical
when the collector is unused; stage rendering/import behavior is unchanged.

`whale_visual.[ch]` validates the nested owner visual/shape/map package and
scatters source-order morph samples into UV-split render vertices. Its caller
provides reusable scratch/output arrays, with no per-frame allocation.
`whale_gpu.[ch]` adds a native update/draw interface using the existing EC Basic
renderer and hardware-validated CPU texture uploader. It resolves material-packed
GPU triangle vertices back to the correct render vertices, updates positions,
flushes their cache and preserves UVs. Sampled normals remain available for the
future original lighting path; they are not replaced with flat normals. The
caller must retire prior GPU work before update/destroy, update once per frame,
and reuse pending vertices for both eyes. Actor/root transforms belong to the
caller's modelview. Unsupported nonidentity resource transforms are rejected.

This GPU module is compiled but NOT called by gameplay. It has no native raster
acceptance yet. Existing Basic material rendering is not a claim that original
lighting, alpha/draw ordering or full whale appearance is complete.

## Verification

Required cumulative host, exact-owner pose/import and native build PASS.
Independent raw-owner audit compares every runtime mapped vertex's original
position/normal bytes, all1380 ordered triangle corners with source indices/UVs,
all five material fields and the four source texture references. It decodes
original polygon streams directly instead of accepting the converter's own
output as the oracle. Runtime checks sample both banks at quarter-frame intervals
and verify source-identity scatter and unchanged UVs. Seven malformed maps are
rejected. The optional collector is also checked for unchanged default output.
Existing pose/terrain/texture gates ran as required; no new duplicate emulator
run because the unreferenced renderer is linker-discarded.

Private visual package154160B SHA256
65c72deb77fcd9bf84b3bf922da97257abf869f4c38d2b666b26f7dc61173aab
is /private/tmp/sa3ds-wca65-whale-visual.sa3dspak, not installed on shared SD.
Native binary remains470768B SHA256
013d05779a47d8d3171e8e3e9ea737356fa48e60808ea7fdc0dda8d9948be642.
Its optional Sonic lighting remains raster-rejected/inactive; the inspected
Desktop preview remains WCA56. No retail or physical Old3DS acceptance claim.

Next: source actor/root transforms and phase/effect scheduling, native lifetime/
trigger/history/CCL services, then original pier/camera execution. A natural
whale encounter and full chase remain absent. CP6 and full CP1–CP9 are OPEN.
