# 2004 PC ↔ Steam core physics parity evidence

Checkpoint: **0.0.8.8.5**

This document records static evidence from two exact user-owned SADX
executables. No executable bytes are included in the checkpoint archive.

## Locked profiles

### Steam

- file: `Sonic Adventure DX.exe`
- size: `90,922,000`
- SHA-256: `e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6`
- image base: `0x00400000`

### 2004 US PC

- file: `sonic.exe`
- size: `55,549,952`
- SHA-256: `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
- image base: `0x00400000`

## Player parameter table: byte-identical

Steam table:

- VA `0x025D9B90`
- stride `0x84`
- count `8`

2004 table:

- VA `0x009154E8`
- stride `0x84`
- count `8`

The complete `8 × 0x84 = 0x420` byte region is identical between the two exact
profiles:

- compared bytes: `1,056`
- differing bytes: `0`
- shared table SHA-256:
  `3f69076118fcc6a77c55e600eb337944eb6562e5b0979104b58058ad64ba5107`

This covers Sonic, Eggman, Tails, Knuckles, Tikal, Amy, Gamma and Big.

The corresponding initializers independently show the same table contract:
character ID from offset `+0x09`, multiply by `0x84`, add table base, copy
`0x21` dwords.

## Ground acceleration

Known entries:

- Steam `PGetAcceleration` equivalent: `0x004DBE50`
- 2004 `PGetAcceleration`: `0x0044C270`

The 0.0.8.8 Steam evidence gate already verified a selected steep-surface
branch. 0.0.8.8.5 located the corresponding 2004 references and confirmed the
same semantic numeric values:

| Semantic label | Value |
|---|---:|
| up threshold | `0.10000000149` |
| cross/unstick comparison | `0.60000002384` |
| speed threshold | `1.15999996662` |
| steep threshold | `-0.40000000596` |
| weight multiplier | `5.0` |
| steep threshold | `-0.30000001192` |
| weight multiplier | `0.80000001192` |
| steep threshold | `-0.10000000149` |
| weight multiplier | `0.40000000596` |
| orientation threshold | `0.5` |
| force multiplier | `4.22499990463` |
| orientation threshold | `0.69999998808` |
| orientation threshold | `0.87000000477` |
| side multiplier | `1.39999997616` |

The 2004 build stores these selected constants as single-precision values;
the Steam compiler references equivalent promoted double constants in this
branch. Their semantic values are equal.

## Air acceleration

2026-10-22 direct PC2004 instruction check: `PGetAccelerationAir` calls the
general-turn helper `0x00443C50` at `0x0044BB22`. Its airborne status branch
(`0x00443CCE..0x00443D23`, status low bits zero) uses explicit f32 constants
at `0x007DFFF0` (0.8999999761581421) and `0x007DD568`
(0.10000000149011612): transformed local velocity * 0.9 + old local velocity
* 0.1 on all three axes. The port incorrectly used 0.99 here. Grounded
turn-inertia rules are separate and unchanged. The existing Break/Inertia
verifier now checks this call, status branch, three mixes and constants.

Known entries:

- Steam `PGetAccelerationAir` equivalent: `0x004DDEC0`
- 2004 `PGetAccelerationAir`: `0x0044B9C0`

The two exact builds show matching physics-field accesses and thresholds in the
checked path:

| Semantic | Player-work offset / value |
|---|---:|
| DisableControl mask | `0x4000` |
| weight | `+0x114` |
| air_resist_air | `+0xEC` |
| air_resist_y | `+0xF4` |
| air_resist_z | `+0xF8` |
| run_speed | `+0xC4` |
| back-angle threshold | `0x6000` |
| air_break | `+0xE8` |
| steering-angle threshold | `0x1000` |
| air_accel | `+0xDC` |

The compiler output is not byte-identical, but the checked algorithmic
structure is strongly aligned.

## What this lets SA3DS do

For the checked core movement components, 2004 PC can become the primary
mod-support/extraction source without forcing a reset of the existing Steam-
derived SA3DS movement baseline.

The project should keep both source adapters:

```text
SADX_SOURCE_2004_US    -> primary extraction/mod ecosystem
SADX_SOURCE_STEAM      -> independent comparison/reference
```

Both should feed the same neutral SA3DS physics representation.

## What this does not prove

Do not promote this to a blanket statement that every physics-related behavior
is identical. Exact collision support probes/order, input cadence, action-state
code, Spin Dash charging, camera behavior and platform wrappers remain separate
verification targets.
