# SA3DS 0.0.9.1E-WIP9 — 12-run White Zone Flight Final Candidate

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
