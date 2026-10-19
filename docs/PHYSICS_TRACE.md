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
