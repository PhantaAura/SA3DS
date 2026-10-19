# SA3DS 0.0.9.1E-WIP9-MacFix1 — Host test portability fix

Parent: `SA3DS_0.0.9.1E-WIP9_FinalFlight_CodeOnly.zip`
SHA-256: `0b883615a46006cdff36179d787130234c7de350e5233a4eda6c84f0c3b5905a`

Problem: macOS Clang fails with an implicit declaration of `mkdtemp` in `tests/test_owner_pair_loader.c`. All tests before that point in the user's screenshot passed.

Modification: add Apple-only `_DARWIN_C_SOURCE` before system headers, retaining `_XOPEN_SOURCE 700` and all runtime logic. Updated `ERROR_HISTORY.md` to prevent recurrence. No gameplay, GPU, physics, owner assets or animation behavior changed.

Test: run `bash tests/run_sa3ds_current_host.sh` first. Then run `bash ./build_0_0_9_0G.sh --sd` with devkitARM installed. Use `RUN12_OLD3DS_TEST.md` for owner-side conversion and Old 3DS hardware checks.

This is NOT a 0.0.9.1E visual release gate; code-only test hotfix. No hardware result is claimed.

Verification in this session: `CC=clang bash tests/run_owner_pair_loader.sh` PASS; `clang -D__APPLE__ -std=c11 -Wall -Wextra -Werror ...` Linux-host simulated Apple branch PASS; `CC=clang bash tests/run_sa3ds_current_host.sh` PASS. Actual macOS SDK build and native 3DS build still pending.
