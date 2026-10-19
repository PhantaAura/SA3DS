# SA3DS 0.0.9.1E-WIP7 — 0G Visual Guard Reintegration

**Run:** Overall 10 / recovery pass 7, Tuesday Oct 13 2026, 05:00 EDT.

**Verified parent:** 0.0.9.1E-WIP6A Owner Visual Preflight, SHA-256 `546761910f0cfb8281a288002e0c51ccc271bb0c0f7ebda6fdfdcd109e98e207`, ZIP integrity PASS. No older baseline substituted.

**Source-level finding:** 0G retained the White Zone parity HUD, trace and camera but had not inherited WIP1-WIP6 renderer/loader/animation reset fixes applied to 0F. This could cause a future 0G hardware build to omit safety work. No hardware corruption was observed.

**Integrated files:** `runtime_0_0_9_0G/source/sonic_gpu.c`, `runtime_0_0_9_0G/include/sonic_gpu.h`, `runtime_0_0_9_0G/source/lab_gpu.c` now match validated guarded 0F counterparts. `runtime_0_0_9_0G/source/main.c` selectively integrates visual, geometry, 17-binding ACTION preflight, same-tick zone reset pose refresh, and GPU/texture diagnostics while preserving 0G parity HUD, trace capture, reference camera and input. Added `tests/test_0g_visual_reintegration.py`; extended cumulative runner. Adjusted original 0G title assertion for explicit WIP7 banner without dropping existing behavior checks.

**Additional sanitizer:** GCC UBSan GPU batch and geometry C host tests PASS.

**Tests:** baseline `bash tests/run_sa3ds_current_host.sh` PASS; after edit initial failure only from stale banner assertion, corrected and full cumulative host suite PASS; new 0G native source integration test PASS. All these are host tests/source checks, **not** a native `.3dsx` build.

**Owner data:** Original sonic.sa3c/sonic.sa3t not available in current mounts, Library/conversation title search, or connected Drive title search. No Sega assets distributed.

**Fidelity:** 0G integration SOURCE-BACKED PARTIAL, synthetic host safety tests PASS. Original Sonic appearance, retail animation transitions, Old 3DS performance and 1:1 physics remain UNVERIFIED or HARDWARE/RETAIL TRACE PENDING.

**Release gate:** 0.0.9.1E NOT PASSED. devkitARM/libctru unavailable, no new `.3dsx`, Old 3DS hardware untested.

**Next scheduled run:** 2026-10-13 06:00 EDT (overall Run 11), inherit WIP7 and prioritize native 3DS compile readiness/actual owner visual validation without abandoning White Zone.
