# SA3DS current checkpoint — Azahar recovery and PC2004 flat-ground trace v2

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
