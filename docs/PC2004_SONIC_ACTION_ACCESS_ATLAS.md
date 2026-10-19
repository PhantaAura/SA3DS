# PC2004 `___SONIC_ACTIONS` Direct-Access Atlas — 0.0.9.0B

Exact source: supported 55,549,952-byte `sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

The new scanner finds direct constant table-slot accesses where the executable:

1. loads global `___SONIC_ACTIONS` at `0x038F6E7C`, then
2. immediately accesses `[table + constant displacement]`.

## Result

- raw occurrences of the absolute global address: **193**
- narrow immediate constant-slot accesses proven: **190**
- direct action indices observed: **0, 1, 2, 3, 4, 5, 6, 7, 20, 77, 100, 122, 123**

Frequency in this narrow pattern:

- 0 → 2
- 1 → 76
- 2 → 2
- 3 → 6
- 4 → 23
- 5 → 15
- 6 → 11
- 7 → 6
- 20 → 1
- 77 → 1
- 100 → 2
- 122 → 4
- 123 → 41

## What this proves

Those indices are directly referenced by code in the exact executable.

## What this does **not** prove

It does not identify any index as "idle", "walk", "run", "jump", "Spin Dash", etc. The accesses include many different game/event contexts. Dynamic/indexed action accesses are intentionally not inferred by the scanner.

This atlas exists to constrain the next reverse-engineering pass once `CHRMODELS_orig.dll` is available, not to assign fake animation names.
