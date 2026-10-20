# Physics traces — format v6

Trace rows are emitted after each deterministic simulation tick. v6 preserves
all v5 input/position/velocity/action/ball fields and appends general-surface
diagnostics:

- `surface_forward_x/y/z`
- `surface_right_x/y/z`
- `support_distance`
- `surface_flags`
- `surface_tangent_speed`

`local_forward_speed` and `local_side_speed` are now measured in the current
surface tangent frame rather than assuming world Y is always up. On flat ground
this is equivalent to the previous yaw-local representation.

The trace comparator does not assume either input file is authoritative. A
reference trace becomes evidence only when its source/version and capture
method are documented.

## White Zone parity trace v2

The interactive/native White Zone trace appends camera yaw, post-deadzone
analog values, local velocity before and after the ground step, acceleration,
requested force, resistance, turn selection, braking/zero-cross flags, and the
PC2004 local-Z jog-threshold stop flag. These are emitted after each fixed 60 Hz
simulation tick and do not alter player state.

Comparison results must be labelled `RETAIL TRACE VERIFIED`,
`SOURCE-EQUATION VERIFIED`, `PORT-INTERNAL REGRESSION`, or `NOT VERIFIED`.
