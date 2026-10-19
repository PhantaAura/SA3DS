# SADX 2004 PC collision/support-order evidence — 0.0.8.8.7

## Profile

`sonic.exe`, 55,549,952 bytes,
SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

## Function boundaries checked

- `PResetPosition`: `0x0043EE70..0x0043F000`
- `PGetInertia`: `0x00443650..0x004437FF`
- `PGetBreak`: `0x00448E50..0x004491DF`
- `PGetAcceleration`: `0x0044C270..0x0044CDEF`
- `PSetPosition`: `0x0044CDF0..0x0044D49A`

## 1.16 reference ownership

Direct absolute references to the exact float at VA `0x007E0178`
(`1.159999966621399f`):

- `PGetAcceleration`: **4**
- `PSetPosition`: **0**
- `PResetPosition`: **0**

This is why 0.0.8.8.7 no longer labels the earlier `normal_y < 0.3 &&
speed < 1.16` collision-side forced fall as exact 2004 behavior.

## Exact PGetAcceleration branch constants

- `0x007DD568`: `0.1f`
- `0x007DD584`: `0.60000002f`
- `0x007E0178`: `1.16f`
- `0x007E00E4`: `-0.4f`
- `0x007E01A8`: `-0.3f`
- `0x007E0064`: `-0.1f`
- `0x007DCCFC`: `5.0f`
- `0x007DCE5C`: `0.8f`
- `0x007DD550`: `0.4f`

The verifier also checks instruction signatures around these references so a
matching float elsewhere cannot accidentally satisfy the evidence gate.

## High-level order evidence

0.0.8.8.6 already proved the ordinary Sonic action handlers call:

- Spin Dash: `PGetRotation -> PGetBreak -> PGetSpeed -> PSetPosition -> PResetPosition`
- Roll: `PGetRotation -> PGetInertia -> PGetSpeed -> PSetPosition -> PResetPosition`

0.0.8.8.7 therefore moves the portable arbitrary-surface contact refresh after
movement rather than querying ordinary grounded support twice before the motion
step.

## PResetPosition behavior checked

On the normal path, `PResetPosition` calls `PConvertVector_G2P` at
`0x0043EEC4`, writes the local vector back into the player speed fields at
`0x0043EF9D`, clears per-frame fields, and updates `Up` through either the
support-normal calculation or the explicit `1.0f` upright path.

## What this does not prove

It does not yet give SA3DS a byte-for-byte reconstruction of the retail
collision resolver's body shape, wall/ceiling probes, dynamic collision, or all
contact classification. Those remain the 0.0.8.8.8 evidence target.
