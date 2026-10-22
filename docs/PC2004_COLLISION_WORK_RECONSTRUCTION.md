# SADX 2004 PC collision-work reconstruction — 0.0.8.8.9

## Evidence authority

Primary authority is the exact supported 2004 US executable fingerprint listed
in `SOURCE_PROFILE.txt`. Public X-Hax `SADXStructsNew.h` symbols are used only to
cross-check human-readable field names for the offset layout proven against the
binary.

## The 0x84 collision work block

`PInitialize` allocates a zeroed `0x84`-byte block and stores its pointer at
`playerwk+0x64`. Its verified portable layout is implemented as
`SA1PC2004CollisionWork` in `pc2004_collision_work.h` with compile-time offset
checks.

The layout is:

| Offset | Field | Size |
|---:|---|---:|
| 0x00 | radius | 4 |
| 0x04 | pos | 12 |
| 0x10 | spd | 12 |
| 0x1C | tnorm | 12 |
| 0x28 | find_count | 2 |
| 0x2A | selected_nmb | 2 |
| 0x2C | yt | 4 |
| 0x30 | yb | 4 |
| 0x34 | angx | 4 |
| 0x38 | angz | 4 |
| 0x3C | normal | 12 |
| 0x48 | normal2 | 12 |
| 0x54 | onpoly | 12 |
| 0x60 | pshbk | 12 |
| 0x6C | anaspdh | 12 |
| 0x78 | anaspdv | 12 |

Total: `0x84` bytes.

## Normal helper initialization

The checked normal `PSetPosition` helper consumes `[playerwk+0x64]`, initializes
`tnorm=(0,1,0)`, copies task X/Z angle state, loads player parameter `height`,
multiplies by `0.5f`, and uses the result as `csts.radius`.

Sonic's checked parameter record has `height=10.0f`, therefore:

`retail csts.radius = 10.0 * 0.5 = 5.0`

## Candidate/contact pipeline

The helper calls the broad polygon collision routine at `0x00453820`. That
routine checks positive `csts.radius` and internally computes `radius * 0.5`.
Candidate results then reach one of the contact-arbitration functions at
`0x0043CCE0` or `0x0043D180`. Both read `csts.find_count` and contain a `0x2C00`
normal-angle cutoff. Accepted surface properties pass through
`HandleSurfaceFlags` at `0x0043CB90`, which accumulates the surface attributes
and selects movement multipliers for specific flag bits.

This is strong evidence that retail player collision is a candidate-driven
polygon resolver rather than a single floor ray.

## What 0.0.8.8.9 does not fake

The complete internal polygon iteration and contact-response algorithm has not
been translated wholesale into neutral SA3DS code. In particular, this pass does
not invent semantics or write ordering for `normal2`, `pshbk`, `anaspdh`, or
`anaspdv` where the binary evidence is incomplete.

The Movement Lab records its accepted synthetic contact into `normal` and
`onpoly` as a diagnostic mirror only. `find_count=1` in that diagnostic means
"one neutral contact accepted by the lab"; it is not a claim that retail always
finds exactly one candidate.

## Why the lab still stands 1 unit from geometry

The original Movement-Lab course was authored around a 1-unit visual/support
stand-off. Retail Sonic's reconstructed csts radius is 5.0. Replacing one with
the other without rescaling/re-authoring the whole synthetic course would mix
coordinate conventions and break the established test fixture.

0.0.8.8.9 therefore keeps them deliberately separate:

- `5.0` = reconstructed retail Sonic csts radius/evidence state
- `1.0` = synthetic Movement-Lab fixture stand-off

Real-stage collision calibration begins once Emerald Coast LandTable geometry is
imported. That is the right place to validate the full neutral polygon solver
against actual SA1 scale and edge cases.

### Emerald Coast bridge (Checkpoint B continuation)

The imported-stage adapter still feeds the inherited one-unit support-center
controller. Its task-foot origin is therefore `controller_pos - up * 1`.
This is an explicit port-coordinate bridge, not a recovered retail body.

Exact PC2004 `0x4915F0` loads `playerwk+0x11C` (`center_height`), transforms
`(0,center_height,0)` through `PConvertVector_P2G`, then adds task position.
Sonic main calls it at `0x49BEFA` before display; `0x494A0F..0x494A1A` reads and
translates that center. Original parameter-table entry 0 contains
`center_height=5.400000095367432`, distinct from collision `height/2=5`.

The stage model now uses this parameter-derived center instead of aligning a
Stand-pose AABB minimum. Its unit-length contact-frame axes also orient the
model on slopes. Exact retail angle smoothing/air orientation are unresolved.
The bounded planar wall sweep uses the recovered five-unit csts radius/center,
bridged from the controller support point. It is not a replacement for the
retail polygon arbitration, edge/corner or body-probe algorithm.

An inclined-wall regression also demonstrates an adapter-ordering defect:
floor resolution occurs before the wall sweep, which can push a grounded actor
below the floor already accepted that tick. The adapter now restores the
one-unit signed support distance only when the actor is still grounded and an
actual imported floor is available, removing inward normal velocity. Airborne
actors and missing support are not snapped. This geometry-consistency repair
is PORT-INTERNAL REGRESSION evidence, not a recovered retail contact formula.

Focused anchor/radius regressions are in `tests/run_emerald_coast_collision.sh`;
exact executable witnesses and parameter values are checked by
`tools/verify_pc2004_stage_scale.py`. Neither changes mesh/stage scale, movement
constants, or White Zone's synthetic support convention.

## Imported-terrain crossing regression (2026-10-21)

Gate A probing at source parent `0b9153c` exposed endpoint-only landing:
344/344 descending probes on actual resident floor-triangle centroids missed
support on their first 60 Hz tick (input velocity Y=-16). For example source
instance 40 / triangle 2 moved from Y=22 to Y=6.08 across its Y=9 floor.
All 1,731 near-vertex/centroid static support probes already passed; this was
not evidence for changing scale, gravity, or contact distances.

The existing stage adapter now searches its bounded grid along the tick's
segment and stops an airborne descending actor at the earliest finite floor
crossing of its inherited one-unit support plane. It preserves the triangle's
normal/flags, updates the existing contact frame/action, and removes inward
normal velocity. Zero-time contact is excluded so departing a ledge does not
reattach to the polygon behind the player. Ascending motion, missing floors,
and actors already below the plane do not attach.

This is **PORT-INTERNAL REGRESSION** evidence, not a PC2004 solver formula.
Remaining tick displacement is conservatively stopped at impact; retail
remaining-time response, joint floor/wall arbitration, corner/edge probes and
the one-unit support bridge remain unresolved. No source movement constants,
Spin Dash equations, animation rules, renderer or uploader were changed.

`tests/run_emerald_coast_collision.sh` now includes these geometric probes
alongside the inherited controller/wall/slope tests. Pass the private owner
collision package as its single argument to repeat the 685-triangle scan.

## Finite edge and shared-support calibration (2026-10-21)

The face-only five-unit wall sweep left a reproducible hole at the finite edge
shared by non-coplanar wall triangles. An initial exact-owner diagnostic found
40 failures among 42 angled shared-wall pairs. This is an adapter defect: the
recovered `height/2=5` radius cannot be represented by infinite face planes
alone when their finite projections miss at a corner.

At package load the adapter now classifies triangle topology. Smooth shared
wall diagonals are suppressed; exposed or sharp wall edges receive bounded
finite-capsule sweeps. Upward shared edges are separately marked only when both
normals are continuous (`dot > 0.9`). The contact query visits every spatial
cell intersecting its finite range, bridges only those marked support seams,
and never turns a boundary ledge into support. Grounded wall resolution then
revalidates imported support, detaching instead of retaining stale grounded
state when no finite floor exists.

Owner-package deterministic results within the adapter's explicit +/-512
opening-area domain:

- 504 angled-corner approaches: zero penetrations inside the five-unit edge
  body;
- 694 adjacent-slope crossings: zero false airborne transitions;
- 50 boundary-ledges: zero retained-support states;
- 1,731 static support probes and 344 swept descents: zero regressions;
- 600-tick movement replay: pass.

These finite edge capsules, topology thresholds and one-unit shared-support
bridge are **PORT-INTERNAL REGRESSION** corrections. They are not asserted to
be the missing retail `normal2`/`pshbk` candidate arbitration. The exact
PC2004 collision-work verifier still proves the 0x84 work layout, five-unit
Sonic radius, candidate call chain and accepted-surface path; it does not prove
this portable edge response algorithm.

## Freeze decision

Core movement math and retail collision architecture are frozen for the
pre-character phase. Exact polygon/contact edge parity remains an explicit
real-stage calibration gate, not a reason to delay Sonic model/animation work.
