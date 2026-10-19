# SA3DS current checkpoint — PC2004 fixed-rate animation reconstruction

**Verified source parent:** Pass11B source-motion audit.

## 2026-10-19 integrated pass

- Verified the owner PC2004 `sonic.exe` as SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
  and analyzed it with REA using Ghidra 12.1.4 (x86 image base `0x00400000`).
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
- Kept the provisional `0.5` fallback for properties 9/10/11 because their
  runtime-owned multiplier inputs are not yet reconstructed. Retail transition
  blending is also still unresolved and was not guessed.
- The CPU-swizzled texture upload path, owner-pair loader, renderer, hierarchy
  code, animation selection, White Zone, and hardware diagnostics are unchanged.

## Verification status

- Exact-executable AnimData reconstruction: PASS (146 rows).
- Deterministic animation timing tests: PASS, including fixed versus dynamic
  rate classification and same-tick stability.
- Pass11B independent raw-DLL pose oracle: PASS (223 nodes, 7 frames, 14,049
  scalar comparisons, maximum absolute error `4.91638197e-09`).
- Full owner-data-free host regression suite: PASS.
- Native Old 3DS SD-mode build with devkitARM/Citro3D: PASS. Output
  `dist/SA3DS_Pass11B_Audited.3dsx` is 239,556 bytes with SHA-256
  `5a504f5ecee14b0a0cdafe5364d741cc6268c9fca707e69e90ea21dd7b9dd669`.
  The only compiler warning is the deliberately inactive legacy GPU-transfer
  helper; the runtime still uses the CPU-swizzled upload path.
- Azahar 2126.0, Old 3DS mode, Vulkan: PASS with locally generated owner-only
  SA3C/SA3T on the emulator SD. All 16 CPU uploads completed, the boot trace
  recorded `First frame rendered/submitted`, and White Zone displayed Sonic at
  60 FPS. This is emulator evidence, not physical-hardware evidence.
- Physical Old 3DS / Old 3DS XL: NOT RUN for this checkpoint. The inherited
  CPU-upload first-frame hardware log remains the latest physical evidence.

## Remaining objective

Recover the property 9/10 external speed inputs and the retail transition blend
state, then implement velocity-driven Walk/Run/Jump/Roll frame progression and
cross-action pose blending without altering physics.

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
