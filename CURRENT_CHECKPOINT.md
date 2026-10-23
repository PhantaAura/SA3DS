# SA3DS current checkpoint — Emerald Coast combined rendering and partial playability

## 2026-10-23 — performance isolation, moving landing and slope Roll

Parent `0b7b714`; inherited local binary excluded. Renderer, CPU uploaders,
collision, scheduler, charge timing and verified movement constants preserved.

Performance comparison used detached temporary builds and a private app/profile/
SD copy at `/private/tmp/sa3ds-perf-lab.LjfKRj`; original session preserved.
The copied Azahar2126.0 executable matches installed SHA-256
`595d3aa891755d061f3b4f7d9f3da715ef3764ec22a4b20a463cf663c1d65751`.
Matched Old3DS/Vulkan, CPU100%, frame-limit100%, resolution1x, vsync/async
presentation/disk shader cache enabled, copied/warmed cache, identical owner
assets and600-tick Emerald Coast replay/spawn/camera. Loading excluded from
steady-state samples. No video recording. Manual checks are not tick-equivalent
comparisons. Full timed seven-scenario matrix remains incomplete.

| Build | Exact artifact SHA-256 | Inspected performance |
| --- | --- | --- |
|1f5ca2c rebuilt|68a87067ec18e6fe7a4d324a341b715606248571823ab695bf7ad2a18a7cc6ce|Initially60FPS/100%; repeat7–34FPS/11–57%|
|4105360 rebuilt|a654dd7595e82895b1d474ea60e9f67fa8f4d778e0069bd62175be0b8130143e|31–59FPS/52–98%|
|0b7b714 archived|23f177cd98b4cd02f0681a6d0b1b9174c17e6e180ffe1f6d03572822d4cacddc|Slow too; compilation/test overlap makes this sample confounded|

The rebuilt old artifact matches its historical known-good hash. Old-build
A/repeat traces match all600 ticks exactly; intermediate/current traces also
match exactly, zero position/velocity error. A normal test-process stop initially
left an old instance alive; ambiguous overlap was excluded, then only one
isolated process was retained. Private samples/logs/traces remain outside Git.
During slowdown macOS reported load183.97,117 running processes,6757.5MiB swap
used and high WindowServer activity. This establishes shared environmental
variability, NOT a proven single cause or a unique new-controller regression.
No gameplay rollback or renderer workaround was justified.

Low Power Mode remained1 for AC and battery. Owner approved an AC-only test;
on resumption the Mac was on battery97%, and `pmset -c` required root and made
NO change. Neither setting changed; no power restoration is outstanding.
Controlled AC-disabled testing remains pending AC connection/authorized change.
Do not attribute recovery or slowdown to Low Power Mode without that comparison.

Source-backed changes (saved export/direct disassembly; no new live REA claim):

- `0x494FF0` and `0x49796E..0x4979A2`: nonzero signed forward landing uses
  ground selector9–13, not unconditional19. Idle/movement may interrupt19;
  removed provisional property4 lock, preserving Uncurl29/1/2 and state2 blend.
  Input-only stationary bypass, impact classification and full idle counters
  remain partial; stationary19 remains explicitly provisional.
- `0x496F00` entry status0x500 retains0x100 through ordinary release. Existing
  grounded Charge/Roll now passes this bit to the recovered surface kernel.
  PGetBreak is unchanged; qualifying slope inertia uses0.0002 drag, not0.008,
  with exact double0.98 boundary. Four real instance74 triangles83/85/94/95
  at forward5 retain about0.039 speed on the tick versus the old specialization.

Validation: focused host/19 scenarios, cumulative current-host suite, exact
Spin Dash/Break/Inertia verifiers PASS. Animation128 timeline replays,
11 moving-landing cases and2 interruption/timing cases PASS. Owner600-tick
replay,4 slope equations,36 launches,36 charge durations,12 exits,6 retained
rolls,12 air-turn ticks/landing plus1731 supports/344 descents/504 corners/
694 slope transitions/50 ledges PASS. Raw-source ACTIONs1/22/122/123 pose oracle
PASS:56196 scalars, maximum error4.99206543e-9. These are source-equation and
port-internal checks; genuine retail runtime traces remain unavailable.

Final Old3DS build PASS:274276 bytes, SHA-256
`a10a8c504b975f1e57cbe92fb3af5aadd1c9f2a1ef89bc670711bcad92cff10a`.
Oct23 Azahar final-build combined Emerald Coast beyond5336 GPU frames,
29382KiB linear free,45 stage/42 Sonic draws; native HUD15ms includes sync.
Replay and live run/jump/charge/release exercised. Final replay first differs
from0b7b714 at frame286: moving landing request19->9, forward0.0296011604;
all600 positions/velocities identical. Trace SHA-256
`5c9ffb81ec580083fea3c00cd2de77567b61a20db5a394a8c48ef9b870d69951`.
Inspected final EC59–60FPS/99–100% with42–43FPS/70–76% live/capture dips;
White Zone movement/jump/charge responsive at56–60FPS,30346KiB free.
Thus60FPS remains reachable without rollback, but stable performance recovery
is NOT certified; guest timing does not prove host FPS. Power state differs
from Oct22, so these are not a controlled before/after power experiment.

Remaining: AC performance gate, stationary impact/idle state, provisional
camera/contact and slope-air semantics, genuine retail traces. A targeted audit
also identifies an unfixed inverted-Up normal/forward-force axis interpretation;
see existing Spin Dash evidence note. `PHYSICS_1TO1=false`; hardware untested.
The disposable benchmark process was closed after saving evidence; the original
emulator session was resumed. Original SD data/control mappings were not changed.

## 2026-10-22 continuation — charge braking and Uncurl request arbitration

Parent `4105360`; recovered the interrupted controller pass without replacing
existing systems. Only the inherited local `.3dsx` was dirty on entry and is
excluded from the source commit. Renderer, CPU uploaders, stage collision,
movement constants, charge accumulation/release and scheduler are unchanged.

- Flat charge movement now reuses the recovered PGetBreak/AddVelocity kernel
  already used on imported terrain, after rotation and before zero-cross
  integration. Initial forward speed 5 gives 4.788, not the old approximation's
  4.86. Signed/zero/lateral cases and an independent 80-tick equation fixture
  guard the actual integrated path; previous source fails the new regression.
- Both Roll exits propagate original Uncurl29. Exact metadata supplies the
  29/ACTION22 -> 1/ACTION122 -> 2/ACTION123 chain. Changed jump/charge/fall
  requests interrupt property4 outside an active blend, as PlayAnimation does;
  moving selection can replace action1. Idle/landing selection remains partial.
- Exact executable signatures verify selector preservation, changed-request
  arbitration and NextAnim writing request/index. Evidence is the exact private
  PC2004 binary, direct disassembly and saved Ghidra export. REA opened the
  executable with Ghidra12.1.4, but its bounded live decompilation/xref probe
  remained pending and was cancelled: no live-analysis success is claimed.

Final focused host/19-scenario suite PASS; cumulative
`bash tests/run_sa3ds_current_host.sh` PASS. Owner collision regression PASS:
600-tick replay, 36 launches, 12 signed Roll exits now checking request29,
6 retained rolls, 36 charge-duration cases, 12 air-turn ticks plus landing;
inherited 1,731 supports, 344 descents, 504 corners, 694 slope transitions and
50 ledges. Exact Spin Dash and Break/Inertia verifiers PASS. Tick-stability
128 replays PASS; owner binding preflight 19 bindings/57 poses PASS.

Extended the existing raw-source Pass11B oracle to ACTIONs1/22/122/123:
223 nodes x 7 frames x 9 scalars each, 56,196 total comparisons PASS.
Maximum absolute errors respectively 4.91638197e-9, 4.98352049e-9,
4.91638197e-9 and 4.99206543e-9. This verifies sampled poses, not retail
movement trajectories or complete animation timing.

Final Old-3DS native build PASS: 274,488 bytes, SHA-256
`23f177cd98b4cd02f0681a6d0b1b9174c17e6e180ffe1f6d03572822d4cacddc`.
Azahar2126.0 Old-3DS/Vulkan loaded that build with external owner assets.
Emerald Coast combined rendering passed 4,967 GPU frames; linear free
29,382 KiB, native HUD15ms. Partial/full input, charge/release, roll exit,
jump interruption and landing exercised. Final native trace has 3,687 rows:
71 charge-animation14, 8 Roll15, 21 Uncurl29, 1 action1, 15 landing19.
At ticks3187/3202/3203 it records 29 -> 1 -> Walk9; the first transition
is exactly15 ticks, moving selection then interrupts action1. Tick224
interrupts Uncurl with jump/action14. Trace SHA-256
`2a5aa3b47e78648944f97743e5978f04cc19ea9404e7f282abb25b7646c29d5d`
at the external SD `traces/emerald_coast_trace.csv`.

This session's emulator performance is degraded: inspected final Emerald Coast
views 19–24 FPS at roughly32–40% speed, including23 FPS/39% in the final
4,967-frame view. The previous executable was also slow before this pass;
cause is not established and no60FPS result is claimed. Native60Hz scheduling
is unchanged; performance certification remains OPEN.

White Zone on the same final build rendered all16 Sonic textures and accepted
partial/full movement, jump/air steering and charge/release. Its final short
trace contains192 rows:23 charge,5 roll and2 airborne; max charge10.3999987,
max horizontal speed10.3167982. Linear free30,346 KiB; inspected active views
22–26 FPS (roughly36–43% speed), also degraded. Native boot log confirms
repeated Sonic draws and frame-end returns. Temporary Select/Circle modifier
bindings restored to N/D and verified in the emulator configuration.

Remaining: complete idle/landing selectors, dynamic collision impulses,
rolling/contact and slope-air semantics, known partial-terrain gap near z413,
provisional follow camera and genuine retail trace comparisons.
`PHYSICS_1TO1=false`; physical Old3DS untested. No full controller-parity claim.

## 2026-10-22 continuation — jump selection before movement

Parent `1f5ca2c`; inherited local `.3dsx` remains excluded. Fixed normal jump
selection before ground acceleration/turning and before charge entry in both
existing controller paths. Surface jumps no longer receive held-jump boost on
their initiation tick. Exact PC2004 action-1/2/5 jump calls, local-Y impulse
and timer-reset signatures verified; action 4 deliberately ignores jump.
No collision, renderer, input constants, pose, or texture-upload changes.

Focused host suite + 19 scenarios PASS; full current host suite PASS;
Spin Dash and Break/Inertia exact-executable verifiers PASS. Added eight flat
launch cases and 36 owner-terrain normal/roll/turn launches; charge-jump
immunity covered throughout the 36 held-charge duration cases. Inherited
600-tick replay, 1,731 support probes, 344 descents, 504 corners, 694 slope
transitions and 50 ledges PASS. The previous source fails the new flat launch
regression; corrected source passes. Comparisons are source-order/kernel
regressions, not captured retail trajectories.

Old-3DS build PASS: 273,512 bytes; SHA-256
`a654dd7595e82895b1d474ea60e9f67fa8f4d778e0069bd62175be0b8130143e`.
Azahar 2126.0 Old-3DS/Vulkan ran that build past 11,300 GPU frames, showing
56–60 FPS during inspected gameplay, 29,382 KiB linear free. Live partial/full
movement, acceleration, turning, braking/reversal, jumps/landing, charge/release,
subsequent roll-jump and reset exercised. Native trace: 3,091 complete rows,
89 airborne, 81 charge, 23 roll, two landings; jump on charge release remains
Roll, subsequent roll jump becomes Airborne with jump_ticks=0/request14.
Trace SHA-256 `e0b93d904ab9102ef26ecfc7fc5e35370e062cf1b90f5d4bae776c0754dbb47b`
at the external SD `traces/emerald_coast_trace.csv`. Prior trace backed up at
`/private/tmp/sa3ds-pre-launch-fix-trace.csv`. Temporary mappings restored.
White Zone on the same executable rendered and accepted movement, jump and
air steering at 60 FPS / 100%, with 30,346 KiB linear free. Pose/model import
was unchanged, so no new pose-oracle run was needed; the parent pass's exact
owner oracle remains recorded below.

Remaining: Uncurl29 request propagation, complete rolling/slope-air/contact
semantics and genuine retail trace comparison. Existing partial collision gap
near z=413 and provisional follow camera remain. `PHYSICS_1TO1=false`;
physical Old 3DS untested. No global controller-parity claim.

## 2026-10-22 PC2004 controller, Spin Dash ordering and charge animation

Source parent `7541180`; only the pre-existing local `.3dsx` was dirty on
entry and remains excluded from source commits. Collision/renderer code was
not changed in this pass. `PHYSICS_1TO1=false`.

- Exact executable SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
  verified. REA opened it with Ghidra 12.1.4; bounded headless decompilation
  attempts did not return usable output and were cancelled. The separate GUI
  bridge returned BACKEND_UNAVAILABLE at localhost:8080. No live decompilation
  or retail execution is claimed. Findings use the saved private Ghidra export
  plus direct disassembly and exact-byte/constant verifiers.
- Air steering now uses `0x443C50`'s airborne 0.9 transformed + 0.1 old-local
  velocity mix, replacing 0.99. `PGetAccelerationAir` calls it at `0x44BB22`.
  Ground turn-inertia and local-Z stopping remain unchanged.
- Corrected the earlier timer-order research: `0x49AA34` calls Sonic_Act1
  before timer increment `0x49AB01` and movement dispatch `0x49AB29`.
  Charge entry moves immediately; held transitions inspect the previous
  timer; release runs roll inertia that tick; timeout dispatches normal
  movement. Entry/release finish at timer 1. Charge constants stay unchanged.
- Roll termination now tests signed forward speed < run_speed before motion
  in both flat/surface paths (`0x4930D0`, caller `0x497458`). A negative
  high-magnitude roll exits; equality survives until the next tick.
- Charging selects original animation 14 / ACTION slot 14 via `0x495C30`,
  replacing row 32. Property-10 playback reads live charge * 0.2. Jump restores
  scalar 5 even when its requested animation ID remains 14. Existing pose,
  transition and same-tick non-mutation behavior remains intact.
- Native trace v2 compatibly appends charge/timer/ball/jump fields and records
  actual filtered input on surface/ball ticks. Air local speeds use current
  yaw. Flat-only force columns remain unavailable for surface/ball ticks.

Validation: focused physics suite + 19 deterministic scenarios PASS; full
`tests/run_sa3ds_current_host.sh` PASS; exact Spin Dash and Break/Inertia
executable verifiers PASS. Owner terrain: 12 signed roll exits, 6 retained
rolls, 36 stationary/moving charge-duration cases across six supported starts,
12 air-turn ticks and landing PASS. The inherited 600-tick replay, 1,731 support
probes, 344 descents, 504 corners, 694 slope transitions and 50 ledges PASS.
Pass11B oracle: 223 nodes, 7 frames, 14,049 scalar comparisons, maximum error
`4.91638197e-09`. Old 3DS code-only build PASS: 273,508 bytes, SHA-256
`68a87067ec18e6fe7a4d324a341b715606248571823ab695bf7ad2a18a7cc6ce`.

Azahar 2126.0 Old-3DS/Vulkan ran the exact build beyond 20,900 GPU frames at
60 FPS / 100% in inspected views, with 29,382 KiB linear memory free. Native
Emerald Coast trace: 4,033 completed rows; 43 half-input and 106 full-forward
rows; 48 charge rows all request 14, 91 roll rows; one live jump/landing.
Stationary charge entry 2/timer1 released to forward `10.3140535`/timer1/request15;
moving entry charge `2.59170318` released to forward `6.93577051`/timer1/request15.
Trace SHA-256 `df473cec44288148dc3eb1c6b87b8c60bc68951829921010bc469bde44a7cee8`
at the owner's external SD `traces/emerald_coast_trace.csv`. These are port
runtime observations, not retail comparisons. Both native charge groups pass
source-equation checks over 46 held rows: maximum charge error `3.52e-08`,
animation-frame error `9.58e-07`. White Zone on the same build rendered and
accepted partial/full movement, jump and air-turn input at 60 FPS / 100%,
30,346 KiB linear memory free. Temporary Select/analog-modifier mappings were
restored to N/D; no replay/autoboot marker was introduced.

Remaining limits: exploratory moving charge from z=400 lost support on held
tick 4 near `(-0.07654,-18.48795,413.30515)`; z=375 can reach the same partial
contact area on release. Interior controller fixtures use supported starts
-50/200/250/300/325/350. Do not promote these tests to full collision parity.
Retail Uncurl request 29 propagation, charge-jump eligibility, flat charge
kernel details, complete rolling/contact status paths and retail runtime
traces remain open. Next objective: source-backed charge-jump/Uncurl state
propagation and equivalent retail/port controller traces. Physical 3DS untested.

## 2026-10-21 Gate A finite-edge and terrain-seam continuation

Source parent `61ac162`. The pre-existing local compiled `.3dsx` modification
was preserved and excluded from source staging. This pass completes the
deterministic Gate A edge suite for the bounded opening-area adapter; it does
not claim the original PC2004 polygon solver or a 1:1 controller.

- A five-unit planar wall face alone leaves Sonic's source-backed sphere body
  able to pass through the finite shared edge of angled walls. On the exact
  owner package, 40 of 42 angled shared-wall pairs reproduced this defect in
  the initial diagnostic. The adapter now marks exposed/sharp wall edges at
  load, suppresses smooth internal mesh diagonals, and performs a bounded
  swept-sphere/finite-edge test after face resolution. The permanent owner
  suite covers both sides and bisector/shallow/steep approaches: 504/504
  corner trials remain outside the five-unit edge radius.
- Upward triangles with continuous normals now mark their shared support edge.
  Contact probes search every grid cell intersecting their finite range and
  accept the one-unit edge support only at those marked seams. True boundary
  edges are never marked. Owner results: 694/694 in-bounds adjacent-slope
  transitions remained grounded and 50/50 boundary departures detached.
  Out-of-domain source edges beyond the adapter's explicit +/-512 query bounds
  are excluded from this runtime-domain test rather than treated as resets.
- Grounded wall resolution now revalidates actual imported support. It repairs
  the inherited leaning-wall ordering case when a floor exists, but changes to
  AIRBORNE when neither finite contact nor the pre-step-height surface query
  finds support. This prevents stale grounded state over empty space.
- Existing owner checks remain clean: 1,731/1,731 static support probes,
  344/344 high-displacement floor crossings and the 600-tick controller replay.
  An imported-ground Spin Dash reached exact-evidence charge `10.399999` and
  transferred `10.399999` tangent speed on release. Exact executable verifiers
  pass for the PC2004 five-unit collision radius/candidate chain and Spin Dash
  entry, increment, timer, release, and action-4/action-5 movement ordering.
- The new corner/edge/support mechanics are explicitly PORT-INTERNAL bounded
  corrections. Movement constants, input filtering, animation, renderer, CPU
  texture upload and `PHYSICS_1TO1=false` remain unchanged. Exact retail edge
  arbitration, roll termination and full collision-flag semantics remain open.

Validation for this exact source:

- Focused synthetic edge suite PASS: 16 support probes, 4 swept descents,
  12 wall-corner approaches, 2 slope transitions and 5 true ledge departures.
  Exact owner-package suite PASS: 1,731 support probes, 344 swept descents,
  504 angled-corner trials, 694 in-bounds slope transitions, 50 runtime-domain
  boundary ledges, the 600-tick controller replay, and imported-ground Spin
  Dash charge/release. ASan+UBSan reported no address or undefined-behavior
  failure (macOS leak detection is unavailable).
- Full owner-data-free host suite PASS. Code-only Old 3DS build PASS: 272,916
  bytes, SHA-256 `a1ed28df13beff0afc954c350ed710a82d309145ebf121d9e1e1b3c60fa90d2d`.
- Azahar 2126.0 Old-3DS/Vulkan ran that exact build beyond 3,400 GPU frames.
  Emerald Coast combined rendering, live acceleration, turn/traversal,
  jump/land, wall blocking and action-button Spin Dash input remained live at
  60 FPS / 100% speed in the inspected views, with 29,382 KiB linear memory
  free. The wall-contact counter advanced while Sonic stopped at visible stage
  geometry. White Zone then rendered and accepted movement/jump input at
  60 FPS / 100%, with 30,346 KiB linear memory free.
- This validates the bounded opening-area port adapter, not the original
  polygon arbitration, full rolling/contact termination, global controller
  parity, or physical Old 3DS behavior. Physical hardware remains untested.

## 2026-10-21 Gate A swept-floor crossing checkpoint

Source parent `0b9153c`. The pre-existing local compiled `.3dsx` modification
was preserved and excluded from source staging. Checkpoint B remains PARTIAL:
this pass fixes one demonstrated terrain-contact failure but does not certify
retail corner/edge arbitration or a 1:1 controller.

- Actual owner collision data reproduced endpoint tunneling on the first tick:
  344/344 descending probes crossed resident support triangles without landing.
  All 1,731 static near-vertex/centroid probes passed, isolating the root cause
  to between-tick floor crossing rather than scale or package geometry.
- Added a bounded grid sweep for descending airborne actors. It selects the
  earliest finite imported triangle crossing at the inherited one-unit support
  plane, carries source normal/flags into the existing surface frame, removes
  inward normal velocity and performs the normal AIRBORNE-to-GROUNDED landing.
  Ascending, below-plane and outside-polygon cases never attach; a zero-time
  hit cannot retain support after leaving a ledge.
- Added synthetic and owner-data deterministic tests for high-displacement
  flat/inclined landing, finite ledge departure, ascent rejection, missing-floor
  rejection and all existing wall/slope/jump behavior. Post-fix owner scan:
  344/344 crossings landed, 1,731/1,731 static support probes remained valid;
  the inherited 600-tick owner controller replay passed.
- Exact-PC2004 collision-work signatures still pass: the 0x84 csts layout,
  height/2=5 radius, polygon candidate call chain and arbitration witnesses.
  The sweep itself is explicitly a PORT-INTERNAL integration correction, not a
  reconstructed retail response formula. Movement constants, Spin Dash,
  animation, renderer, CPU uploader and `PHYSICS_1TO1=false` are unchanged.
- Full owner-data-free host suite PASS. Code-only Old 3DS build PASS: 267,636
  bytes, SHA-256 `9162f909700927eaa1332585569f6310d93652757c00153d86429a97296f43c0`.
  Azahar 2126.0 Old-3DS/Vulkan exercised the exact build beyond 2,000 GPU
  frames: combined rendering, acceleration, turn, jump/land and shoreline
  movement remained live at 59-60 FPS with 29,382 KiB linear memory free.

Remaining Gate A work is deterministic interior/exterior corner coverage,
adjacent-triangle slope-transition stress and actual ledge-route capture. Gate
B controller comparison and Gate C Spin Dash changes remain gated on that work.

## 2026-10-21 Checkpoint B source-backed anchor/water/contact continuation

Source parent `cfbd7ac`; only the prior local compiled binary was dirty at
resume. No unfinished source work was discarded. Final source was host-tested,
natively built and exercised in Azahar; broader Checkpoint B fidelity is OPEN.

- Replaced the Stand-AABB-derived 4.267-unit vertical lift with the original
  Sonic render-center equation: task origin plus rotated
  `center_height=5.400000095367432`. Exact PC2004 `0x4915F0`, called before
  display at `0x49BEFA`, builds the center read by `0x494A0F..0x494A1A`.
  The inherited controller's one-unit support point is explicitly bridged to
  a task-foot origin. Unit-length contact-frame axes orient the model on
  supported slopes; retail angle smoothing/air orientation remain partial.
- The bounded wall sweep now uses the recovered normal PSetPosition center
  and five-unit radius (`height=10`), not a one-unit point near the feet.
  Floor support still uses the inherited controller adapter. Exact retail
  polygon arbitration and edge/corner probes are not claimed reconstructed.
- Fixed a demonstrated adapter-ordering inconsistency: an inclined wall can
  push an already-grounded actor below the floor accepted earlier that tick.
  Restore signed support distance on an available imported floor after walls;
  never snap airborne actors or fabricate missing support. A deterministic
  leaning-wall regression covers this PORT-INTERNAL correction. A longer
  pre-fix native wall trial fell unsupported; its complete retail/contact cause
  is not claimed isolated. The corrected native wall trial remained grounded.
- Preserved signed-negative land visibility; split GPU material batches by
  original water land class too. Supported water draws after ordinary stage
  and Sonic with depth testing, no depth writes, verified source-alpha blend,
  and SDK state restoration. Non-water alpha/foam and the separate fade path
  are not guessed; full original transparent sorting remains pending.
- Focused collision/anchor tests PASS; owner 600-tick controller replay PASS;
  visual reader PASS on synthetic and exact owner package; new executable GPU
  host test PASS (actual batching/draw source, mocked SDK, shared materials,
  texture reuse, depth masks, state restoration/failure cleanup).
  Full `bash tests/run_sa3ds_current_host.sh` PASS with the new GPU gate.
  Exact executable scale/anchor/water signatures PASS; source coordinates
  remain unchanged (223 nodes, 4,525 Sonic vertices, 480 stage instances,
  22,237 stage vertices). Pass11B oracle PASS: 14,049 scalar comparisons,
  max error 4.91638197e-09. No physics movement constants were changed.
- Final build ran in Azahar 2126.0 Old-3DS/Vulkan beyond 2,800 combined frames. Live
  half/full movement, turn/coast, jump/landing, slope traversal, camera rotation,
  reset and wider wall blocking were inspected. The sustained reverse/wall
  trial reached 27 wall contacts while grounded. Sonic remained visible in the
  tested shoreline run/landing views. Native CSV contained input Z=0.493589729
  and Z=0.987179458, with airborne and grounded samples. Stable views measured
  60 FPS, 29,382 KiB linear free; boot/warm-up showed lower FPS and is not a
  steady performance result. White Zone live movement/jump passed at 59–60 FPS
  with 30,346 KiB linear free in the same exact executable.
- Final `tools/build_3ds.sh` PASS: local code-only 266,220-byte `.3dsx`, SHA-256
  `a67f45ee96362732f7b79e2d96f9971b790181e0795193ef4da1473cb1030428`.
  Temporary keyboard mappings were restored and the test-only autoboot marker
  removed; no replay marker was active during live controls. An interrupted
  automation attempt was restarted after the owner confirmed keyboard access.
- REA tools absent in this session; pinned doctor attempt failed with npm
  `ENOTFOUND registry.npmjs.org`. Used the existing Ghidra export and direct
  exact-executable disassembly/signatures, never Hopper. No live decompile
  success is claimed. Physical Old 3DS NOT RUN; `PHYSICS_1TO1=false`.

**Next:** expand deterministic/native coverage of terrain edges, wall/floor
corners and unsupported departures before closing Checkpoint B. The five-unit
wall body is source-backed, but the support bridge, contact arbitration, spawn
and follow camera remain partial/provisional. Water depth-write treatment is
corrected, not full water/foam/sorting/lighting parity. No broad-stage guarantee
is inferred from the inspected route. CPU uploaders, movement kernels and
animation sampling remain unchanged. Compiled binary and owner data are excluded
from the source commit; do not repeat extraction or the historical stall audit.

## 2026-10-21 Checkpoint B visibility / live-input validation (parent `26fe846`)

- Fixed an original-game discrepancy: PC2004 `FUN_0043a900` only appends land
  entries with signed-negative COL+0x20 flags to its draw list. Exact executable
  bytes at `0x0043AA5B` (`8b472085c0795a`) skip non-visible entries to the
  36-byte loop stride. The existing GPU subset now requires bit 31 too, instead
  of drawing collision-only geometry. Collision residency is unchanged.
  Current opening render: 30 instances, 45 batches, 5,053 triangles, 15,159
  vertices, 25 textures / 688 KiB. This does not reconstruct the other original
  visibility masks, frustum tests, transparency ordering or sea/background.
- Fixed pre-Citro3D boot-menu presentation: present TOP and BOTTOM with
  `gfxSwapBuffers`, rather than only BOTTOM. The updated native menu visibly
  runs and selects Emerald Coast and White Zone. Citro3D frame submission and
  both CPU texture uploaders remain unchanged. The earlier combined GPU stall
  was not reproduced; its historical root cause remains UNKNOWN.
- Added owner-only `tools/verify_pc2004_stage_scale.py`: exact executable hash
  and draw-filter signature; independent raw source comparison of all 223
  Sonic NODE positions/scales and 4,525 converted local vertex positions;
  all 480 act-0 instance positions/scales/flags/centers/radii and 22,237 local
  vertices match source float bytes. Converted angles agree within 2e-6 rad.
  No map or Sonic rescaling is introduced. Stand source-pose height is 9.856
  units. This proves coordinate preservation, NOT every retail transform,
  camera, body or pose behavior. The one-unit contact body, fixed 4.267-unit
  presentation lift and spawn `(0,2.963578,-50)` remain explicitly provisional;
  full retail contact-body reconstruction is still needed.
- Exact updated build in Azahar 2126.0, Old-3DS/Vulkan: menu-selected combined
  stage/Sonic ran past 3,300 GPU frames; live partial/full input, turning,
  coasting, jumping/landing, repeated reset and wall blocking were exercised.
  CSV recorded 36 ticks at input Z=0.493589729 and 60 at Z=0.987179458; source
  contact normals included `(0.071704,0.996831,-0.034449)` and
  `(0.119183,0.991656,-0.049137)` while moving across the beach. A reset/down
  sequence increased wall contacts from 0 to 132 and blocked forward progress.
  These are PORT-INTERNAL REGRESSIONS, not retail traces or complete collision
  semantics. Following camera remained usable; it is not retail CAM behavior.
- Measured 29,382 KiB linear free; usually 59–60 application FPS at 99–100%
  speed, with observed 50–56 FPS dips. CPU uploads completed. White Zone loaded
  separately in the same build and accepted live movement/jump at 60 FPS.
  Temporary emulator keyboard mappings were restored after testing.
- Verification: full `bash tests/run_sa3ds_current_host.sh` PASS (including
  package, collision, CPU-upload and rendering guards); owner collision
  600-tick replay PASS; raw scale/signature audit PASS; Pass11B pose oracle PASS
  (223 nodes, 7 frames, 14,049 scalar comparisons, max error 4.91638197e-09).
  `tools/build_3ds.sh` PASS: local code-only 264,404-byte `.3dsx`, SHA-256
  `ed7a4ff51ab0dfc8d4b2b73b6747fdbb2bd7c6ca660d44afd286104bfc2c1e20`.
- REA/Ghidra live import/decompile attempt timed out during autoanalysis; no
  successful live decompile/xref is claimed this pass. Used the saved Ghidra
  export plus direct exact-executable byte checks, never Hopper.
- Basic bounded playability is demonstrated, not full Checkpoint B fidelity:
  shoreline/model occlusion, incomplete material semantics, partial wall/body
  contacts, provisional model anchor/spawn/camera and limited residency remain.
  Next: reconcile retail body/ground anchoring against the visible shoreline,
  then validate contact/visual agreement without retuning movement. Physical
  Old 3DS testing NOT RUN; `PHYSICS_1TO1=false` remains unchanged. No owner game
  payload or compiled binary is included in this source checkpoint.

## 2026-10-20 Checkpoint B integration (parent `6c5b0ee`)

- Re-enabled stage and Sonic submission in the same frame, sharing one stage
  view matrix. The previous next-frame GPU stall **was not reproduced**: bounded
  begin/draw/end markers returned at frames 0, 1, 2, 60, and 180; later frames
  continued. No GPU root cause or texture-upload fix is claimed. Both existing
  CPU uploaders and the per-draw Sonic arena are unchanged.
- Added a strict reader for the separate owner-generated `COLLISION` package
  (`SA3DECOL` v1). It preserves original instance transforms/identities/land
  flags, selects act 0 and the opening residency, and indexes 16 instances / 685
  triangles into a 16x16 grid (1,959 triangle references). Runtime queries do not
  scan the full-level triangle list. Retained triangle/index/grid storage is
  approximately 63 KiB; no extra GPU collision buffers are allocated.
- Connected imported triangle floors/contact normals and bounded swept planar
  wall contacts to the existing controller. This is a **partial port contact
  adapter**, not the retail polygon/body-probe solver: upward-floor threshold,
  one-unit body, two-sided wall sliding, edge/corner behavior, and land-flag
  semantics beyond the selected solid bit are not retail-parity claims.
- Provisional spawn `(0, 2.963578, -50)`, yaw 0: support is actual act-0 instance
  74, triangle 49, normal approximately `(0.072, 0.997, -0.034)`. This is NOT a
  recovered retail start position. Reset uses the same imported support. Sonic
  keeps original scale/pose; a fixed Stand-foot presentation offset aligns the
  model with the existing one-unit controller support body.
- Reused the existing 60 Hz controller, animation selector/blending and camera
  yaw. The following view is provisional (40 units behind, 18 above), not retail
  CAM behavior. Movement/animation/texture/physics kernels are not retuned.
- Optional developer replay feeds the existing controller for 600 ticks:
  neutral, half/full forward, turn, coast, jump/release/land, reverse, wall.
  Native traces use the existing CSV format and the stage-specific identity
  `EMERALD_COAST_ACT0_PROVISIONAL`; these are PORT-INTERNAL REGRESSIONS, not retail
  traces. A paused native jump was visibly inspected above the imported beach.
- Combined Azahar Old-3DS/Vulkan measurements: 60 application FPS at 100% speed,
  29,344 KiB linear free, typically 15–16 ms submission diagnostics; 48 stage and
  41–42 Sonic draws. These do not establish physical Old 3DS performance.
- Live Azahar input demonstrated sustained forward running, turning, jump,
  landing, reverse movement and manual camera rotation in the exact final build.
  Simple views measured 59–60 FPS; camera/movement checks also showed dips to
  53 FPS / 89% speed. White Zone rendered at 59–60 FPS and accepted a live jump.
- Full Checkpoint B acceptance remains open: partial deflection was tested by
  the scripted input fixture, not sustained HID input; the complete live
  wall/slope/reset sequence still needs demonstration. Water uses the inherited opaque
  subset, so shoreline/submerged movement can occlude Sonic. No sky/events,
  full-stage residency, camera collision, retail collision or physics parity.
  `PHYSICS_1TO1=false` remains unchanged.

### Local assets / repeatable test

Keep all owner-derived files outside Git under `sdmc:/3ds/SA3DS/assets/`:
existing `sonic.sa3c`, `sonic.sa3t`, `emerald_coast_act0.sa3dspak`, plus
`emerald_coast_collision.sa3dspak` generated by the existing collision importer.
The tested collision package is 735,904 bytes, SHA-256
`211ac5e32ef275328a80df9f3123b41c6bd6b1b3722876fda2dbf5f316d33115`.

Boot `R` opens combined Emerald Coast; `A` still opens White Zone. An owner-side
`emerald_coast_stage_only.flag` keeps the isolated Checkpoint A preview.
`emerald_coast_autoboot.flag` selects stage (empty/other) or White Zone (`W`).
`emerald_coast_replay.flag` enables the optional input fixture; first byte `P`
pauses at replay tick 270 for airborne inspection (SELECT+A resumes). Remove
the test markers for normal boot-menu/live-input use. SELECT+Y resets;
SELECT+X records `traces/emerald_coast_trace.csv`.

### Verification for this integration

- `sh tests/run_emerald_coast_collision.sh`: floor support, jump/landing, swept
  wall, slope, unsupported ground, reset, bounds/NaN/CRC; valid-CRC malformed
  version/model/index/vertex/offset packages rejected.
- Exact owner package: 600-tick existing-controller replay passes with finite
  poses, jump and reattachment. Retail execution/traces were not captured.
  The earlier native 600-tick run used the initial z=200 trial spawn; the final
  inland-spawn build was inspected at replay tick 270 and then tested with live
  controls. The final inland 600-tick host replay also passes.
- `bash tests/run_sa3ds_current_host.sh`: PASS, including inherited physics,
  rendering/CPU upload guards and Pass10A checks.
- Pass11B exact-owner pose oracle: PASS, 223 nodes / 7 frames / 14,049 scalar
  comparisons, maximum error `4.91638197e-09`.
- `tools/build_3ds.sh`: PASS; local code-only 263,920-byte `.3dsx`, SHA-256
  `b12c2030d18cf1f8df242902e8feebe813cc2e0d56ffe68f2b945f15da5830c4`.
  Compiled binaries and owner data are deliberately excluded from this commit.

---

**Verified source parent:** commit `f3f9b79be6ac1bf10efb31b5c7e3b36abce4bb60`.

## 2026-10-20 Emerald Coast Checkpoint A

- Added a strict native reader for the existing owner-generated visual
  package contract: `.sa3dspak` v1 plus `SA3DVIS` v1, `SA3DMATL` v1, and
  embedded `SA3T`. It validates chunk presence/order/size, every region,
  model/mesh/instance/material/index reference, texture range, and finite
  transform/vertex values before GPU allocation.
- Added boot-menu `R` for the source reconstruction's internal act 0 (the
  displayed Emerald Coast Act 1). White Zone remains on `A`; Pose Lab remains
  on `L`. An empty owner-side SD marker can autoboot the preview for emulator
  automation without embedding game data.
- Reused Ninja object translation/rotation-flag/scale composition and the
  hardware-proven CPU Morton texture uploader. No display-transfer upload was
  reactivated.
- The preview keeps a 500-unit opening subset: 45 original instances, 48
  material batches, 5,694 triangles, 17,082 expanded static vertices, and 25
  of 103 source textures. Geometry is pre-baked once and grouped by material;
  stage UVs remain un-clamped and textures use repeat wrapping. Culling is
  disabled pending flag-semantics evidence; raw material diffuse and texture
  association are honored, while transparency ordering and lighting are not
  claimed.
- Azahar 2126.0 in Old 3DS/Vulkan mode visibly rendered the connected authentic
  owner-derived geometry and water. The validated stage-only run measured
  59–60 emulator application FPS at 99–100% speed on the final build, with
  29,915 KiB linear memory free and 688 KiB of resident stage textures.
  Physical Old 3DS performance remains unmeasured.
- Checkpoint B (Sonic, imported collision, controller, and follow camera) is
  intentionally not claimed. The visible renderer was isolated from Sonic
  submission after their combined first frame stalled the next GPU frame;
  White Zone still provides the unchanged validated Sonic runtime.

### Verification

- Owner act-0 package: 4,349,696 bytes; SHA-256
  `5fbb164a27337381cea897f4dc2be01906e669aa70804b86ce0945e77efc7bce`;
  exact counts 183 models, 578 meshes, 480 instances, 22,237 vertices, 58,794
  indices, 237 materials, and 103 textures.
- Native reader synthetic fixture and exact owner package: PASS, including CRC
  corruption rejection.
- Full owner-data-free host regression: PASS, including all inherited White
  Zone, animation, pose, GPU-arena, loader, scheduler, and Pose Lab gates.
- Pass11B raw owner-DLL pose oracle: PASS (223 nodes, 7 frames, 14,049
  scalars; maximum absolute error `4.91638197e-09`). The exact executable
  SonicAnimData reconstruction also passed.
- Old 3DS SD-mode devkitARM build: PASS. The code-only `.3dsx` is 251,900
  bytes with SHA-256
  `c43e23c27c7a1026bd142a681c753b40c340ab682163a258fa2d6a4466ad7564`.
  The sole warning is the deliberately inactive legacy GPU-transfer helper.
- Visual Azahar Checkpoint A: PASS on the exact final artifact for authentic
  geometry/texture display and stable repeated frames. Physical Old 3DS:
  NOT RUN.
- White Zone remains available through menu `A` and all its host gates pass;
  a fresh interactive White Zone run on this final artifact was not completed.
- `PHYSICS_1TO1` remains false. No proprietary package content is in Git.

### Next objective

Generate/install the separate owner collision package, add a spatially bounded
triangle-contact query for the same opening subset, establish a source-backed
provisional spawn, and then reintroduce Sonic under a measured combined GPU
budget for Checkpoint B.

---

# Prior checkpoint — Azahar recovery and PC2004 flat-ground trace v2

**Verified source parent:** commit `9130f0f316bda3daa2a3592741c2dfa3a61b9f09`.

## 2026-10-20 Azahar recovery and local-Z inertia-stop checkpoint

- Started from clean commit `9130f0f316bda3daa2a3592741c2dfa3a61b9f09`.
- Reproduced boot marker `000` in Azahar 2126.0 and inspected the actual
  startup order. The marker is written before the interactive three-mode boot
  menu; `romfsInit` is intentionally deferred until a mode is chosen. The
  apparent current/parent "failure" was therefore a waiting menu, not a crash
  and not a physics regression. Selecting `A` starts full White Zone.
- Reverified the private PC2004 executable as SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
  REA opened that exact file with Ghidra 12.1.4 / IBM Semeru Java 21, but a
  new live decompilation timed out while whole-program analysis was starting.
  This pass therefore does not claim a new live query. It reuses the previous
  checkpoint's live `PGetAcceleration` (`0x0044C270`) and turn-helper evidence,
  and independently checks this pass's findings against exact instructions in
  the executable. Hopper was not used.
- Recovered the ordinary grounded update call order at `0x0049AD34` through
  `0x0049AD54`: movement helper, `PGetAcceleration` (`0x0044C270`),
  `PGetSpeed` (`0x00443F50`), `PSetPosition` (`0x0044CDF0`), then
  `PResetPosition` (`0x0043EE70`). The verifier decodes and checks every rel32
  target against the exact executable.
- Found a confirmed discrepancy in `PGetSpeed`. PC2004 applies the
  `0.87 * jog_speed` stop band to local-Z (side) inertia as well as forward
  speed. SA3DS previously retained a decaying lateral remainder until sign
  crossing. The existing flat-ground integrator now matches the positive and
  negative PC2004 branches. With Sonic's parameters, side speed `1.0` damps to
  `0.6` on tick 1 and snaps to `0.0` on tick 2 instead of retaining `0.36`.
- Extended the existing CSV trace compatibly to version 2. It now records
  post-deadzone input, filtered magnitude/direction, camera yaw, before/after
  local velocity, acceleration/resistance contributions, turn decision, brake
  and zero-cross branches, and the local-Z stop-band event. Added neutral,
  partial-input, quick-turn, camera-relative, and side-threshold scenarios.
  Comparator output now distinguishes `RETAIL TRACE VERIFIED`,
  `SOURCE-EQUATION VERIFIED`, `PORT-INTERNAL REGRESSION`, and `NOT VERIFIED`.
  No retail runtime trace was captured; the new numerical fixtures are
  source-equation verified, not proof of global 1:1 parity.

### Verification

- Exact-executable signature/call-target verifier: PASS.
- Focused White Zone protocol: PASS, 17 deterministic scenarios; the five new
  cases report `SOURCE-EQUATION VERIFIED`.
- Full owner-data-free host regression: PASS.
- Old 3DS SD-mode devkitARM build: PASS. Updated `.3dsx` is 242,140 bytes,
  SHA-256 `49c358e4e5d05648c2b98f9a348c16757f96524533630e8b4e37ac3c0fc82027`.
  The only warning remains the deliberately inactive legacy GPU uploader.
- Azahar 2126.0, Old 3DS mode, Vulkan: PASS on that exact updated artifact
  with owner-generated SD assets. The latest boot segment passed `romfsInit`,
  completed all 16 CPU-swizzled texture uploads, initialized Sonic's GPU data,
  and logged first-frame submission. White Zone rendered Sonic, accepted
  movement/action input, and measured 59–60 FPS.
- Physical Old 3DS / Old 3DS XL: NOT RUN. `PHYSICS_1TO1` remains false.

### Remaining objective

Capture an independently instrumented PC2004 retail flat-ground trace with
matched initial state and inputs, then use the version-2 comparator to identify
the earliest real per-tick divergence. Recover the remaining opaque
PSetPosition/PResetPosition body-probe and collision-response semantics before
promoting any broader physics-parity claim.

## 2026-10-20 PC2004 flat-ground turn-inertia checkpoint

- Started from clean commit `56535c281b67c91bcf76bcace6dcfee431ab1f1a`.
- Reverified the owner PC2004 `sonic.exe` as SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
  REA's Ghidra 12.1.4 provider produced a live `PGetAcceleration`
  (`0x0044C270`) decompilation and 40 incoming references. The direct GUI
  bridge was unavailable and the first headless analysis exceeded its startup
  deadline; subsequent live Ghidra decompilation and reference queries passed.
  Hopper was not used.
- Confirmed that the existing PC2004 input builder already preserves analog
  magnitude: per-axis 3072 deadzones precede a clamped cubic radial curve.
- Recovered flat/upright turn inertia from `0x00443C50`, `0x00443DF0`, and
  `0x00443E60`. General and slow turns retain
  `0.9900000095 * old_local + 0.00999999046 * transformed`; the prior native
  implementation had those operands reversed. Quick turn writes the saved
  velocity transformed into the new player frame; the prior implementation
  restored the stale local vector.
- Corrected the existing flat-ground helper without changing movement
  constants, input routing, collision, animation, rendering, or the CPU
  texture-upload path. `PHYSICS_1TO1` remains false.

### Verification

- Exact-executable signature verifier: PASS, including all three recovered
  turn helpers and their retail constants.
- Focused flat-ground tests: PASS, including near/partial/full analog input,
  braking, reverse input, quick turn, and source-derived general-turn output.
- Full owner-data-free host regression: PASS (19 scenario regressions plus all
  inherited input, physics, animation, renderer-contract, scheduler, package,
  and importer tests).
- Old 3DS SD-mode devkitARM build: PASS. Updated `.3dsx` is 241,120 bytes,
  SHA-256 `7506efdc0cce30275414a0e93b8a813581398249d6d02ff1eb1a1fd052a58a4c`.
- Azahar 2126.0, Old 3DS mode, Vulkan: BLOCKED in this session. The updated
  build and a clean copy of the parent `56535c2` artifact both emitted only
  boot marker `000` and stopped before `romfsInit`; therefore this is not
  evidence of a physics regression, but no current emulator gameplay or
  visual validation is claimed.
- Physical Old 3DS / Old 3DS XL: NOT RUN.

### Remaining objective

Capture an equivalent PC2004 runtime flat-ground trace for numerical per-tick
comparison, then recover camera-relative input/update ordering and the
remaining non-flat acceleration residual without changing verified physics.

## 2026-10-19 PC2004 transition/blending checkpoint

- Reverified exact PC2004 `sonic.exe` SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
  The REA/Ghidra bridge was unavailable (Ghidra's default Java 25 process
  failed); targeted recovery used the exact Ghidra 12.1.4 C export and raw
  executable instructions. Hopper was not used.
- Recovered `PlayAnimation` (`0x0044A800`) transition dispatch and its local
  transition-motion builder (`0x0043F2A0`). Same-root destination rows below
  transition speed 1 blend a frozen source pose toward destination frame zero.
  Position/scale are linear; rotation uses the shortest signed 16-bit BAMS
  path (`0x004383D0`). Root changes and speed-1 rows switch immediately, and
  an active blend is not interrupted by a newly requested index.
- Recovered the ground selector at `0x00495CD0`: IDs 9/10/11/12/13 are chosen
  from signed local speed plus five times the acceleration residual at the
  exact jog/run/rush/dash thresholds. The prior runtime skipped IDs 11 and 13.
- Replaced the one-tick landing placeholder with property-4 playback. Land
  (ID 19, ACTION 11, 16 frames) advances until `frames - 1`, then installs
  `NextAnim` 0 without blending. Soft-versus-hard landing selection remains
  provisional because the presentation bridge does not expose the exact
  pre-contact vertical state used by Sonic's action switch.
- Added one caller-owned transition pose buffer to the existing renderer; no
  per-frame allocation or duplicate animation system was introduced. The CPU
  texture uploader, physics, `PHYSICS_1TO1=false`, Pose Lab, owner loader,
  hierarchy evaluator, and White Zone remain unchanged.

### Verification

- Focused transition/timing and shortest-BAMS tests: PASS.
- Real owner-generated SA3C preflight: PASS, 17 bindings / 51 pose samples;
  exact ACTION roots and frame counts agree.
- Pass11B raw-DLL source-pose oracle: PASS, 223 nodes, 7 frames, 14,049
  scalars, maximum absolute error `4.91638197e-09`.
- Full owner-data-free host regression: PASS.
- Old 3DS SD-mode devkitARM build: PASS. Updated `.3dsx` is 241,152 bytes,
  SHA-256 `cfcd6225d43d8868816ab2ab5321bcae9754ea36652a0552c65f015049d91830`.
- Azahar 2126.0, Old 3DS mode, Vulkan: PASS on the updated build with owner SD
  assets. It ran at 59-60 FPS; boot logging confirmed all 16 CPU texture
  uploads and first-frame submission. Live HUD/visual checks exercised Stand,
  Walk/Run (ID 12/ACTION 6 reached), Jump/Spin (14/14), Land (19/11), and
  return to Stand without a freeze or invalid pose.
- Physical Old 3DS / Old 3DS XL: NOT RUN for this checkpoint.

### Remaining objective

Recover the exact soft-land (ID 2) versus hard-land (ID 19) state input and
the non-flat acceleration residual. Add a source-oracle transition-pose dump
for numerical retail comparison beyond the verified formulas and synthetic
shortest-path test; current emulator validation establishes runtime behavior,
not complete animation parity.

## 2026-10-19 integrated pass

- Reverified the owner PC2004 `sonic.exe` as SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
  The live REA/Ghidra bridge was unavailable for this pass, so the targeted
  recovery used the exact Ghidra 12.1.4 C export plus raw instructions from
  that executable. Hopper was not used.
- Confirmed animated hierarchy traversal at `0x004050C0`: parent local
  transform is applied before model draw and child recursion; sibling traversal
  occurs after matrix pop. The existing ACTION 1 source-pose oracle still
  agrees for 223 nodes, 7 frames, and 14,049 scalars (maximum absolute error
  `4.91638197e-09`).
- Recovered all 146 retail `SonicAnimData` rows from the exact initializer at
  `0x007CEC90`, including action slot, instance, property, next animation,
  transition speed, and animation speed.
- Verified in retail `PlayAnimation` at `0x0044A800` that fixed properties
  advance by their row `AnimationSpeed`. Replaced SA3DS's global provisional
  `0.5` step for those properties with the recovered per-animation rate. Fall
  and Spin Ball now advance at `1.0` frame/tick; Stand remains its proven
  `0.5` frame/tick.
- Recovered the dynamic inputs from `InitCharacterVars` (`0x00442750`) and
  Sonic's ground-animation selector (`0x00495CD0`). Property 9 advances by
  `CharObj2.Speed.x * AnimationSpeed`. Properties 10/11 read the stateful
  `CharObj2.SpindashSpeed`: ground locomotion refreshes it as
  `CharObj2.Speed.x + 20 * EntityData2[0x38]`, while the jump transition at
  `0x00495E60` assigns `5.0`. Property 10 multiplies it by `AnimationSpeed`;
  property 11 uses it directly.
- Integrated those rules into the existing selector and accumulated each
  completed tick's step so later speed changes do not rewrite earlier frames.
  Jump/Spin consequently advances at `5.0 * 0.2 = 1.0` frame/tick regardless
  of later airborne speed. The verified flat/upright physics path supplies the
  cleared zero residual; unrecovered non-flat residual paths retain the flagged
  `0.5` fallback.
  Animation timing remains presentation-only and cannot modify movement.
- The CPU-swizzled texture upload path, owner-pair loader, renderer, hierarchy
  code, animation selection, White Zone, and hardware diagnostics are unchanged.

## Verification status

- Exact-executable AnimData reconstruction: PASS (146 rows).
- Deterministic animation timing tests: PASS, including signed property 9,
  properties 10/11, changing movement inputs, per-tick accumulation, unknown
  residual fallback, persistent Jump/Spin scalar, selection changes, and
  same-tick stability.
- Pass11B independent raw-DLL pose oracle: PASS (223 nodes, 7 frames, 14,049
  scalar comparisons, maximum absolute error `4.91638197e-09`).
- Full owner-data-free host regression suite: PASS.
- Native Old 3DS SD-mode build with devkitARM/Citro3D: PASS. Output
  `dist/SA3DS_Pass11B_Audited.3dsx` is 239,884 bytes with SHA-256
  `83a29554b538a6a44fc0417ed1fee2929462b1eea11b560f0f2fb249227ee71a`.
  The only compiler warning is the deliberately inactive legacy GPU-transfer
  helper; the runtime still uses the CPU-swizzled upload path.
- Azahar was not rerun for this checkpoint. The inherited 2126.0 Old 3DS/Vulkan
  result remains the latest emulator evidence: all 16 CPU uploads completed,
  the boot trace recorded `First frame rendered/submitted`, and White Zone
  displayed Sonic at 60 FPS with owner-generated SD assets.
- Physical Old 3DS / Old 3DS XL: NOT RUN for this checkpoint. The inherited
  CPU-upload first-frame hardware log remains the latest physical evidence.

## Remaining objective

Recover the exact non-flat `EntityData2+0x38` residual behavior and retail
transition-blend state, then implement cross-action pose blending without
altering physics.

## Inherited 0.0.9.1E-WIP9 checkpoint

**Scheduled run:** Overall Run 12/12, 2026-10-13 07:00 America/New_York (EDT).
**Verified parent:** 0.0.9.1E-WIP8 External Owner Pair, SHA-256
`c9428d39e432378acaabf8f6fea22df15b4c2251a832ae35cc1833ad4edaa502`.
Parent ZIP passed independent SHA-256 and ZIP integrity. Previous RUN_STATE,
ERROR_HISTORY, PARITY_STATUS and checkpoint documentation were inspected.

## Integrated changes

- `src/game/white_zone_tick.[ch]`: integer-phase 60Hz simulation scheduler
  independent of rendering; latched input edges, bounded catch-up, reset.
- `runtime_0_0_9_0G/source/main.c`: 0..4 simulation ticks per rendered frame,
  physics/animation/trace/camera follow per tick; pause/resume via SELECT+A,
  frame-drop telemetry and reset. No Sonic physics constants changed.
- `runtime_0_0_9_0G/Makefile`: `ASSET_SOURCE=sd` now selects isolated
  `romfs_sd/`, preventing a locally generated owner RomFS package from being
  accidentally embedded in the SD-mode executable.
- `runtime_0_0_9_0G/romfs_sd/sa3ds/README.txt`: code-only package marker.
- `tests/test_white_zone_tick.c`, `tests/run_white_zone_tick.sh`,
  `tests/test_white_zone_tick_integration.py`: deterministic scheduler and
  runtime source-integration tests, added to cumulative host suite.
- `tests/test_owner_asset_build_mode.sh`: actual GNU Make variable resolution
  and SD package-tree isolation checks.
- Updated state, parity, error ledger and Old 3DS XL reproducibility guide.

## Tests actually executed

1. `sha256sum /mnt/data/SA3DS_0.0.9.1E-WIP8_ExternalOwnerPair_CodeOnly.zip`
   — exact parent hash above, PASS.
2. `unzip -t /mnt/data/SA3DS_0.0.9.1E-WIP8_ExternalOwnerPair_CodeOnly.zip`
   — PASS.
3. `bash tests/run_sa3ds_current_host.sh` before modifications — PASS.
4. `bash tests/run_white_zone_tick.sh` — PASS (integer 60Hz cadence,
   edge latching, multi-step, stall cap, reset and deterministic replay).
5. `gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined
   -fno-sanitize-recover=all tests/test_white_zone_tick.c
   src/game/white_zone_tick.c -o /mnt/data/sa3ds_run12/test_wz_ubsan`
   followed by `/mnt/data/sa3ds_run12/test_wz_ubsan` — PASS.
6. `bash tests/run_owner_pair_loader.sh` — PASS (including SD packaging
   isolation with real GNU Make and stub 3ds_rules).
7. `bash tests/run_sa3ds_current_host.sh` after modifications — PASS,
   all inherited host tests preserved.
8. `make -C runtime_0_0_9_0G ASSET_SOURCE=sd` — BLOCKED by absent
   DEVKITARM, exact Makefile diagnostic. This is NOT a native compile PASS.

## Release gate and evidence

**0.0.9.1E visual foundation: NOT PASSED.** This is a cumulative CODE-ONLY
source candidate, not a proven Old 3DS XL executable. Original owner-generated
SA3C/SA3T, native Citro3D compilation, hardware rendering, retail ACTION
transition timing, actual 60Hz performance and global physics parity remain
unverified. Source-backed partial / host-verified tests must not be conflated
with retail or hardware verification. No Emerald Coast play gate earned.

**Next:** Use `RUN12_OLD3DS_TEST.md` on the owner's Mac and Old 3DS XL.
The 12 scheduled opportunities are finished; no automatic next run.

## 2026-10-15 MacFix2 (source syntax hotfix; NOT a new release gate)
Parent: `0.0.9.1E-WIP9-MacFix1_CodeOnly.zip`, SHA-256 `4e0dbe43f180048f79e90f24f585dfa12e63fdc5b1c8541fabfa1edf422f55ab`.
Observed real Mac devkitARM compiler failure at `runtime_0_0_9_0G/source/main.c:176-180` due to interleaved HUD `printf` strings. Restored two independently terminated HUD calls at rows 27/28 and added native-source lexical regression gate. All other runtime logic is inherited unchanged. Owner must rerun actual native compiler; hardware render still pending.
