# SA3DS 0.0.9.1E-WIP8 — External Owner Asset Pair Loader

**Scheduled run:** Overall Run 11, 2026-10-13 06:00 America/New_York (EDT).

**Independently verified parent:** 0.0.9.1E-WIP7 0G Visual Reintegration,
SHA-256 `a6975cca7b791be96848422438580316f6aa5eab083f97c730514904e1810332`;
`sha256sum` exact match, `unzip -t` PASS. Parent state and ERROR_HISTORY
read before editing; no older source substituted.

**Purpose:** Allow a code-only native White Zone `.3dsx` to be compiled when
owner-generated Sonic packages are only on the tester's SD card, without
weakening the original RomFS owner-data build path. This removes a packaging
obstacle to actual 0.0.9.1E visual testing; it is not a rendering fidelity fix.

**Implementation:** The native 0G runtime now loads a **complete pair** from
`romfs:/sa3ds/` or falls back to a **complete pair** in
`sdmc:/3ds/SA3DS/assets/`, never mixing origins. Defensive file size cap
(16 MiB each), exact-length reads, cleanup on partial failure, source HUD.
`make ASSET_SOURCE=sd` and `./build_0_0_9_0G.sh --sd` bypass only the
compile-time RomFS owner asset presence check. Default RomFS build remains
guarded. Original SA3C/SA3T structural/visual/geometry/ACTION preflight still
runs regardless of source.

**Actual tests and results:**
- `sha256sum /mnt/data/SA3DS_0.0.9.1E-WIP7_0GVisualReintegration_CodeOnly.zip` PASS, exact parent hash.
- `unzip -t ...WIP7...zip` PASS.
- `bash tests/run_sa3ds_current_host.sh` before edit PASS.
- `bash tests/run_owner_pair_loader.sh` PASS: C fixture tests fallback,
  source-pair atomicity, precedence, zero-length files, 16MiB cap, failure
  cleanup; source integration contract; Makefile mode test using stub rules.
- `gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iruntime_0_0_9_0G/include runtime_0_0_9_0G/source/owner_assets.c tests/test_owner_pair_loader.c -o /mnt/data/sa3ds_run11/owner_pair_asan && ASAN_OPTIONS=detect_leaks=1 /mnt/data/sa3ds_run11/owner_pair_asan` PASS.
- `bash tests/run_sa3ds_current_host.sh` after edit PASS, all inherited 0G
  physics, trace, CAM, GPU batch, visual, ACTION, geometry tests retained.
- `bash -n build_0_0_9_0G.sh` PASS.

**Error learning:** Initial new C test did not compile under `-Werror` due to
GCC `-Wformat-truncation` on a 512-byte synthetic test path. Increased the
test path buffer to 1024 bytes; reran PASS. No production bug inferred.

**Not done:** No devkitARM, libctru, Citro3D, `picasso` or `3dsxtool` found
in this execution environment. No native `.3dsx` compiled. No owner SA3C/SA3T
or physical Old 3DS was used. No Sega data included. Synthetic loader tests
are HOST VERIFIED; native integration is SOURCE-BACKED PARTIAL; 0.0.9.1E
visual release gate remains NOT PASSED.

**Next scheduled run:** Overall Run 12, 2026-10-13 07:00 EDT. Inherit WIP8,
validate ZIP/hash and host suite; focus native build readiness and hardware
instructions, avoid false 1:1/boot/performance claims.
