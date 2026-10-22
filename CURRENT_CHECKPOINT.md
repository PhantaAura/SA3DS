# SA3DS current checkpoint — Emerald Coast combined rendering and partial playability

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
