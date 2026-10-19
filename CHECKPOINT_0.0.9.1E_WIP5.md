# CURRENT CHECKPOINT — SA3DS 0.0.9.1E-WIP5

**Verified cumulative parent:** `0.0.9.1E-WIP4 Action Pose Preflight`, SHA-256 `ad00a50be61d22ecbb0b60a82fd938ce7d2715520f5515f133799cd1724dcba2`; `sha256sum` and `unzip -t` passed. Inherits WIP1–WIP4 and the original 0.0.9.1D CAM baseline.

**New:** Every enabled key in all 17 bound ACTIONs is scanned at package load for nonfinite position/rotation/scale components, descending key times, empty enabled tracks and out-of-range key/channel spans. WIP4's start/mid/end pose sampling remains. The diagnostic now includes runtime ID, ACTION slot, channel, track and key. This catches a corrupt key that can be hidden between three sampled poses. Added a **read-only portable host CLI** to validate owner-generated SA3C packages without requiring a 3DS toolchain. No Sega assets embedded, and no physics constants/animation selection/timing changed.

**Host validation:** `bash tests/run_sa3ds_current_host.sh` PASS, `bash tests/run_sonic_action_preflight.sh` PASS; GCC ASAN/UBSAN synthetic preflight PASS. CLI passed a generated 17-binding/51-pose fixture and rejected a deliberately hidden corrupt key. These are **synthetic host tests**, not owner Sonic rendering evidence.

**Pending:** No owner-generated SA3C present in this runtime; no native devkitARM/libctru toolchain; no Old 3DS XL hardware observation. Retail animation transition policy and 0.5 source-frame/tick are **UNVERIFIED**. This release remains **WIP; 0.0.9.1E gate NOT PASSED**. Next: run the host CLI on owner-generated Sonic SA3C and compile/test GPU renderer on actual Old 3DS.
