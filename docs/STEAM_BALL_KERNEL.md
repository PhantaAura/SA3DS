# Ball / Spin Dash kernel — 0.0.8.8.6 evidence boundary

## What changed

The ordinary Sonic Spin Dash constants and core state rules are no longer
working hypotheses for the primary 2004 PC profile. Static verification against
the exact supported 2004 US `sonic.exe` proves:

- entry minimum `2.0f`
- no 10.0 clamp on moving entry
- held compare threshold `10.0f`
- held increment `0.4f`
- no post-add clamp
- timeout at `300` timer frames
- release Action `4 -> 5`
- release copies stored charge to local forward speed
- release resets timer and charge
- timer increments before the charge transition check

The implementation lives in `steam_spindash_evidence.c/.h` and both the flat
and arbitrary-surface state machines use the same rules.

## Important 10.0 detail

`10.0f` is a comparison threshold, not a saturation assignment. The original
routine does approximately:

```text
if charge < 10.0:
    charge = f32(charge + 0.4)
```

This means moving entry values can cross 10.0 by one increment, and repeated
f32 standing increments can settle around 10.4 rather than being clamped to
exactly 10.0. 0.0.8.8.6 has regression coverage for this behavior.

## Movement-order evidence

The primary 2004 action handlers are also statically mapped:

- Spin Dash action 4:
  `PGetRotation -> PGetBreak -> PGetSpeed -> PSetPosition -> PResetPosition`
- Roll action 5:
  `PGetRotation -> PGetInertia -> PGetSpeed -> PSetPosition -> PResetPosition`

The portable flat kernel is compatible with the proven high-level order, but
some collision/contact side effects inside those retail helpers remain to be
ported for a global 1:1 claim.

## Input architecture

Normal contextual ACTION and the restored D-down shortcut are still semantic
aliases into one state machine. The shortcut never receives separate physics;
it only requests the same grounded transition.

## Steam comparison boundary

0.0.8.8.5 proved that the complete player parameter table and selected core
ground/air semantics match between the exact Steam and 2004 profiles. The
compiler-transformed Steam Sonic Spin Dash state routine has not yet been
independently isolated, so the new Spin Dash rules are labeled exact for the
2004 primary source rather than falsely claiming a second binary proof.
