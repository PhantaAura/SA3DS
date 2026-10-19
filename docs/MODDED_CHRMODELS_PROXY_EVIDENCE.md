# Modded CHRMODELS clarification

This resolves an earlier source-profile ambiguity.

`system\CHRMODELS.dll` in the user's modded install is not being treated as the untouched Sega character-data DLL. Its exact fingerprint matches the installed `SADXModLoader.dll` byte-for-byte. The original character module is retained as `system\CHRMODELS_orig.dll` (7,647,232 bytes; SHA-256 `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`).

SA3DS source rule going forward:

1. Use `sonic.exe` to prove the game's expected loader/export interface.
2. Use exact `CHRMODELS_orig.dll` as the original character-data source once mounted.
3. Treat the live `CHRMODELS.dll`/SADXModLoader binary as **mod-behavior evidence**, not as canonical base character geometry.
4. Never run x86 mod-loader code on 3DS; reproduce selected observable mod behavior natively.
