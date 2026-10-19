# Sonic Adventure 3DS — 0.0.9.0C

**Name:** Real Sonic Data Decode

## Status

PASS — code-only checkpoint.

The exact clean PC2004 character DLL is no longer a blocker. Real Sonic model,
hierarchy, material, action, motion and key data are decoded and can be
converted locally into pointer-free SA3C. The exact matching SONIC.PVM is also
validated and decoded by the importer.

## Hard evidence

- CHRMODELS: `76bbaa1b...0c2a5`
- SONIC.PVM: `f8e30dfb...ecbe`
- normal root: 73 nodes
- 149 ACTION slots / 146 populated
- 87 action-hierarchy models
- deterministic SA3C: 1,669,728 bytes from the exact sources/tool revision
- native SA3C validation: PASS
- all inherited physics / 19 Movement Lab scenarios: PASS

## Freeze

Do not reopen frozen physics for character presentation work. Animation remains
presentation-only and must not write gameplay state.

## Next

0.0.9.0D — Real Sonic White-Space Render Gate.
