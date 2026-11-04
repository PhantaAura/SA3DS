# WCA73 — collision edge lookup during moving-region rebuilds

The existing region loader computes edge masks before replacing the current
collision window. Its64-unit grid buckets can contain thousands of triangles;
for every edge it scanned unrelated bucket triangles looking for near-equal
endpoints. This work occurs synchronously during movement. This change targets
that loading cost and preserves the current bounded collision adapter.

Temporary .01-unit endpoint bins filter candidates. Adjacent bins are searched
in all three dimensions. Actual matching still uses the original squared
point-distance<1e-6 predicate, wall/support classification and dot thresholds.
Candidate triangles must belong to the same original edge-midpoint grid cell,
using the original clamped cell calculation/AABB limits. Existence of a matching
neighbor is independent of traversal order. Neither triangle ordering, grid
references, source IDs nor contact equations change.

The index is discarded after marking. On either allocation failure or any
coordinate outside the finite +/-10million index domain, the existing scan
runs before any masks have changed. At the loader's16384-triangle bound, index
storage is at most1,507,328 bytes (three20-byte endpoint records per triangle,
131072 four-byte bucket heads); typical regions use less. This is ordinary
heap memory, not GPU linear memory. No persistent cache or SD payload changes.

Differential check retained the pre-WCA59 independent scan. It matched complete
triangle bytes including both edge masks in128 synthetic classification,
positive/negative bin-boundary and point-tolerance cases plus three original
owner regions. Single host timing pass, not hardware:

| Region | Triangles | Legacy edge pass | Indexed edge pass |
| --- | ---: | ---: | ---: |
| Act0 (0,0) | 685 | .521ms | .231ms |
| Act0 (-768,1024) | 3494 | 11.313ms | 1.489ms |
| Act1 original start (-986.5,-2856) | 473 | .616ms | .130ms |

Required `tests/run_sa3ds_current_host.sh` and `tools/build_3ds.sh` passed.
Owner-only focused comparison used the existing private collision package;
no geometry/import/pose bytes changed, so WCA72's full owner pose/import gate
was not repeated. An initial requested act1(0,0) sample contained no resident
geometry; the final differential run used the actual source start and passed.

Native image473464B SHA256
c4d216883b97133426f09bf3e99339b83287e181b0a31525796c909fc8aa17c5
actually ran Azahar2126.0 isolated Old3DS/Vulkan. Menu, act0 terrain, Sonic and
movement rendered. Movement rebuilt regions(-256,256),974 triangles/45ms,
and(-512,512),1169 triangles/52ms. Post-transition frame retained valid ground
contact near(-759,-6,762). No dense-region native timing or hardware proof.

WCA72 previous candidate's emulator shutdown stopped responding; only that
private process was terminated after normal quit/TERM failed, then WCA73
started cleanly. This is not evidence of an in-game hardware freeze.

The observed Old3DS freeze report remains open. Full rebuild cost includes
model transforms, allocation/grid construction, surface remap and SD logging;
GPU submit stalls remain visible too. Improvement of one pass does not prove
elimination of sudden freezes or retail-equivalent terrain/camera behavior.
