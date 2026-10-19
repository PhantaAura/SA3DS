# CURRENT CHECKPOINT — SA3DS 0.0.9.1E-WIP6A

**Parent:** verified 0.0.9.1E-WIP6 Geometry Preflight, SHA-256 `328098479c6a2fc751309bae81ed44fd8066f47809d261377d0ff8dedfc034a9` (ZIP integrity PASS). This is an **integration revision within the same 04:00 EDT scheduled opportunity**, not a new scheduled run. Inherits all earlier checkpoints through 0.0.9.1D.

**Concrete addition:** `tools/check_sonic_visuals.c` portable read-only owner-side visual+geometry validation CLI; `tests/make_synthetic_sonic_visuals.py` original synthetic SA3C/SA3T pairs; `tests/run_sonic_owner_visual_cli.sh` positive/negative integration gate; cumulative host runner updated. Original owner data are never bundled.

**Evidence and corrected hypothesis:** `tools/build_pc2004_sonic_sa3c.py` hashes CHRMODELS DLL while `tools/build_pc2004_sonic_sa3t.py` hashes SONIC.PVM. A proposed equality check was caught as incorrect **before release** and removed; valid source pairs can have different source hashes. Compatibility is tested using metadata and references, not origin hash equality.

**Tests:** `bash tests/run_sonic_owner_visual_cli.sh` PASS (valid + intentionally different origins; bad UV/metadata/index and missing texture negative cases); `bash tests/run_sa3ds_current_host.sh` PASS; GCC AddressSanitizer/UBSan CLI synthetic PASS. WIP6 inherited host tests passed before changes. No actual owner SA3C/SA3T files were available in this execution. No native `.3dsx` compilation or Old 3DS hardware verification.

**Parity:** HOST VERIFIED synthetic, SOURCE-BACKED PARTIAL for format/geometry. Retail material appearance, original animation transitions, native GPU alpha/UV orientation, hardware performance and 1:1 physics are NOT VERIFIED. The 0.0.9.1E release gate remains NOT PASSED.

**Next:** obtain owner-generated Sonic packages, run the two portable checkers, compile with devkitARM/libctru, inspect actual Sonic rendering on Old 3DS XL. Do not skip ahead to Emerald Coast gameplay.
