# Physics traces — format v6 / native parity trace v4

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

## White Zone/Emerald Coast parity trace v3

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

The 2026-10-24 v3 extension preserves the first 67 columns and appends six
Circle Pad provenance fields: `circle_sample_valid`, raw X/Y, normalized
magnitude/angle and the device full-scale convention. These describe the
physical adapter before the exact PC2004 filter; the existing `move_x/z`,
post-deadzone and filtered fields describe the later stages. Host fixtures set
the device sample invalid. Native samples currently record the practical
libctru full scale of 156 without claiming physical Old 3DS calibration.

The 2026-10-25 v4 extension preserves all first 73 v3 columns and appends
`pc2004_action_state` plus `pc2004_action_state_valid`. The port's broad
`SA1ActionState` is a control implementation detail and is not numerically
comparable with retail `EntityData1.Action`. The new field is written only
from source-reconstructed transitions. A retail comparison aliases retail
`action_state` to this field and fails rather than guessing when validity is
zero. Existing port-internal comparisons continue to compare the broad state.
Reset and verified landing/ball/air transitions establish a value. Live
Ghidra 12.1.4 decompilation of exact-executable selector `0x00494FF0` and its
`Sonic_Act1` caller `0x00496F50` now cover the ordinary action 1/2 boundary.
Action 1 writes action 2 when the processed-analog helper is active. Action 2
calls `0x00494FF0` before movement and writes action 1 only when processed
analog is inactive and signed local forward speed is exactly zero. Therefore a
speed that crosses zero during that tick remains action 2 until the following
tick. Flat and arbitrary-support paths preserve this pre-movement ordering.

Exact executable control flow distinguishes two action-8 entries that the
former trace could not represent. The jump helper `0x00495E60` writes action 8
and requests animation 14. Action 5 instead calls support helper `0x00494F70`;
loss of support with ball status writes action 8 without replacing the current
Roll request 15. SA3DS now records this provenance and retains request 15 when
rolling leaves terrain. This is SOURCE-EQUATION VERIFIED; no retail rows have
yet been captured.

Live REA/Ghidra 12.1.4 now verifies the exact PE32 SHA and decompiles/xrefs the
input filter at `0x0040F170` and `PGetAcceleration` at `0x0044C270`. The older
`0x004C3FC0` input lead is invalid for this executable. Exact instructions still
confirm signed axes shifted by 8, component deadzone 3072 and cubic radial
coefficient `1/(29441^3)`.

`tools/pc2004_retail_capture/` is the portable Windows x86 Frida endpoint. It
hash-gates the owner executable and reads the filter globals without writing
game memory. Capture v2 observes Sonic Act1 `0x00496F50`, but emits only after
Sonic main `0x0049A9B0` returns so movement dispatch and animation arbitration
have completed. Exact instructions establish that motion-work `+0` is the
CharObj2 pointer and world velocity is `+4/+8/+C`; v1's shifted velocity fields
must not be used as retail evidence. v2 also records requested/effective
animation ID, frame and transition provenance. The comparator requires the v2
sidecar, matching digest and post-main timing/animation fields before it can
emit `RETAIL TRACE VERIFIED`, and supports explicit frame shifts for measured
tick alignment. Its synthetic self-test is always `NOT VERIFIED`.

The filter result is snapshotted when Sonic main enters and carries an
`input_filter_sequence`; a later filter invocation therefore cannot be paired
with the state emitted on main return. Missing input provenance aborts capture.
Retail-only provenance columns have no native counterpart, so the comparator
reports them as `UNPAIRED_REFERENCE_PROVENANCE` while still requiring every
shared retail/core movement and animation field on both sides. This supports
the intentionally asymmetric schemas without weakening the numerical gate.

The locally attached Wineskin build runs the exact owner game, but its PE is
hosted inside Rosetta-translated x86_64 `wine64`. REA Frida could enumerate the
process but returned `Unable to access process`; a native x86_64 debugger probe
also stopped the wrapper and was not used for capture. The app was relaunched
unchanged. No retail rows were captured. A Windows x86 Frida endpoint (or a
Wine-native guest injector) remains required; do not relabel schema fixtures.
Live capture aborts instead of granting retail authority if the agent does not
report ready, any instrumentation read fails, or no Sonic Act1 row is observed.

Comparison results must be labelled `RETAIL TRACE VERIFIED`,
`SOURCE-EQUATION VERIFIED`, `PORT-INTERNAL REGRESSION`, or `NOT VERIFIED`.
