# SA3DS 0.0.9.0E.1 — Real Sonic 3DS Runtime Build Fix

Input checkpoint: `SA3DS_0.0.9.0E_RealSonic3DSRuntimeGate.zip`
Input SHA-256: `9bc4aad9c23ea05674f08688128d671833a6764eafc3cdd473949ab5f8f9391f`

## Confirmed fix
The 0E Makefile derives `sonic_shbin.h` from `source/sonic.v.pica` via the devkitARM 3DS shader rule, but 0E `main.c` incorrectly included/referenced `sonic_v_shbin`. E.1 changes the runtime to use:

- `#include "sonic_shbin.h"`
- `sonic_shbin`
- `sonic_shbin_size`

The source-contract regression test now checks the corrected generated symbol/header names and rejects the old `sonic_v_shbin.h` reference.

## Validation
- `tests/test_0_0_9_0E_runtime_source.py`: PASS.
- Full owner-data 0E suite cannot run in this environment because the exact owner `CHRMODELS_orig.dll` and `SONIC.PVM` are not mounted here. This is an environment limitation, not a newly observed code failure.
- devkitARM is not installed in this environment, so no `.3dsx` is claimed from E.1 here. The user's Mac/devkitARM build remains the authoritative next compile gate.

## Distribution audit
No Sega DLL, PVM, generated SA3C/SA3T package, ROM, or extracted Sega texture/audio payload is added by E.1.

## Next gate
Build E.1 on the user's Mac with their verified owner files. If that compiles, perform the real 3DS hardware render test before advancing the official hardware status.
