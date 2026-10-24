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

The 2026-10-22 compatible v2 extension appends `spindash_charge`,
`spindash_ticks`, `ball_form`, and `jump_ticks`. Native Emerald Coast traces
use the same writer. Filtered input now comes directly from the actual input
sample even when surface/ball ticks clear the flat-only debug record;
airborne local speeds use the current yaw rather than a stale support frame.
Force/turn debug columns still describe the flat kernel only; their zeros on
surface/ball paths do not establish zero force. Animation ID/frame and
controller action are captured together after simulation.

The 2026-10-23 v2 extension preserves the first58 columns and appends9:
`animation_requested_id`, `landing_classification_valid`,
`landing_classified_request`, `landing_impact_world_y`, `landing_impact_valid`,
`landing_air_action`, `processed_analog_active`, `processed_analog_valid`,
`idle_counter`. Classification records the Act1 phase; requested ID records
the later main-selector arbitration, which can overwrite hard19 with0 in the
same tick. Active animation may still differ during an original state2 blend.
Impact is retained pre-projection worldY for ordinary static contact only.
The comparator treats the8 integer additions exactly and impactY numerically.
Different column sets are reported, not silently promoted to complete matches.

Retail capture remains blocked on the current ARM macOS host: the exact PE32
x86 owner executable cannot be spawned natively (actual REA/Frida probe:
`Exec format error`); no Wine/x86 Windows runtime is available. The original
file was not altered; the identical executable temporary probe copy was removed.
A compatible owner Windows execution/instrumentation endpoint is needed before
any static-equation/port fixtures can be replaced by genuine retail traces.

Comparison results must be labelled `RETAIL TRACE VERIFIED`,
`SOURCE-EQUATION VERIFIED`, `PORT-INTERNAL REGRESSION`, or `NOT VERIFIED`.
