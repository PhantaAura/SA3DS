# Curated Mod Relevance — Loader Gate

The real-byte loader result clarifies how SA3DS should treat PC mods.

The user's modded installation has a live `system\\CHRMODELS.dll` whose exact
fingerprint matches the installed SADX Mod Loader DLL, while the original
character-data module is preserved as `CHRMODELS_orig.dll`. The proxy contains
code that obtains `CHRMODELS_orig` and requests original character exports.

This is useful architectural evidence, but SA3DS will **not** attempt to execute
that x86 interposition layer on 3DS.

Instead:

1. original `CHRMODELS_orig.dll` is the evidence-gated base character source;
2. curated character/model mods are translated or imported on the host side;
3. behavior mods are reimplemented natively only after their observable behavior
   is understood and tested;
4. the 3DS runtime consumes SA3DS-native packages and feature/profile selections.

This keeps normal SA3DS faithful while still leaving a clean route for curated
Dreamcast Characters / Dreamcast Conversion / Physics Swap support later.
