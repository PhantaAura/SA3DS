# WCA44 — avoid repeated resident collision validation

The owner reports sudden freezes while moving on Old 3DS. This checkpoint
removes one measured source of repeated work; it does not establish that all
reported freezes are fixed on hardware.

The native movement loop recenters its collision window after a 256-unit
margin. Previously each recenter reopened the immutable resident package,
rechecked package/chunk CRCs and scanned all vertices, indices, models and
instances before constructing the local region. No SD package read was needed,
but the full validation still ran synchronously on the movement thread.

A borrowed SA3DSECCollisionArchive now records a successfully validated package
view. Each live region retains it. Residency reuses it only for that region's
same package pointer and size; a newly supplied package takes the ordinary full
validation path. The loader must keep the bytes alive and immutable, as the
native session already does. There is no process-global address cache. Region
construction, contact-edge classification, support mapping, atomic failure,
source coordinates, timing and physics equations retain their prior behavior.
The view adds no transformed geometry cache or owner asset to the repository.

## Checks and measurement

- Required owner-data-free cumulative host gate PASS, including fresh versus
  cached geometry/contact masks/grid equality, player/support preservation and
  corrupt replacement-package rejection.
- Native devkitARM build via tools/build_3ds.sh PASS. Final code-only artifact:
  440076 bytes, SHA256
  b8e87b490269a0bcacdf5c772ce147469cde7552d96ae5a3acaf9afccced1412.
- Exact external owner collision package, five act-0 regions: (-9,4), (256,0),
  (512,0), (768,512), (1024,512), with 685/365/290/1051/827 triangles.
  Host O2 / ffp-contract=off timing: fresh validation+build 13.606–14.553 ms;
  prepared view+build 0.213–1.037 ms. Triangles including both edge masks,
  grid offsets and reference order were byte-identical in each comparison.
  An earlier instrumented run attributed about 12.1–12.4 ms to validation,
  0.04–0.13 ms to transform/grid and 0.17–1.17 ms to contact edges.
  These are host measurements, not Old 3DS timing or steady-state FPS.
- Final artifact booted and rendered Sonic and the complete act-0 terrain in
  Azahar 2126.0 Old3DS/Vulkan. CUA arrow holds did not move Sonic; no native
  region-crossing or hardware improvement is claimed from that boot check.
- Region diagnostics now include buildMs to support an actual hardware check.
  Existing boot diagnostics, CPU texture uploader and owner SD loading remain.

The package remains external in the private SD profile. Temporary profiling
source, generated data and builds stay outside Git. No pose/import changes;
the unchanged owner pose oracle was not rerun. Per-event synchronous SD logging,
contact-edge construction and other movement/render costs remain candidates.
Full physical Old3DS acceptance and full WCA remain open.
