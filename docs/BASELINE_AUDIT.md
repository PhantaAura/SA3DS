# 0.0.7 baseline audit

Archive SHA-256 matched `5152429f55c22523f2684594b9c0c0eab61a53b3dafabc2a7504d14274bbe7d0`. The supplied manifest was read as 6,798 valid inventory rows; its executable row matches the supported 90,922,000-byte Steam fingerprint. The manifest supplies no file contents or executable addresses.

## What existed

- A native libctru Movement Lab with original/simple runway, slopes, bank, ledge/drop/reset behavior and a clean default presentation.
- One deterministic physics step per display loop, placeholder movement tuning, Circle Pad deadzone/normalization, jump, braking, steering, slope/bank response, and fall reset.
- A four-check host assertion program and Old 3DS manual checklist.

## Gaps found

- Portable formulas lived under `runtime/`; input normalization and controller reads lived directly in `main.c`.
- Simulation state used platform-width `unsigned long`; no stable trace schema or reference comparator existed.
- Tests did not exercise scripted scenarios, byte-for-byte deterministic traces, malformed external data, or package loading.
- No source-independent package, neutral importer contract, resource metrics interface, or developer wrapper existed.
- No compiled `.3dsx` was present, consistent with the checkpoint's honest toolchain note.

The first real devkitARM rebuild exposed one 0.0.7 C portability bug: `main.c` used `abs()` without declaring it via `<stdlib.h>`. That was corrected. No formula-level regression bug was proven by the available tests. Repeated upward jump cutting and all tuning remain provisional project behavior rather than verified SA1 behavior.
