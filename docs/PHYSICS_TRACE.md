# Physics traces — format v6 / native parity trace v3

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

Live REA/Ghidra 12.1.4 now verifies the exact PE32 SHA and decompiles/xrefs the
input filter at `0x0040F170` and `PGetAcceleration` at `0x0044C270`. The older
`0x004C3FC0` input lead is invalid for this executable. Exact instructions still
confirm signed axes shifted by 8, component deadzone 3072 and cubic radial
coefficient `1/(29441^3)`.

`tools/pc2004_retail_capture/` is the portable Windows x86 Frida endpoint. It
hash-gates the owner executable, reads the filter globals, and emits one state
row after Sonic Act1 `0x00496F50`; it does not write game memory. The comparator
now requires the capture sidecar and matching trace digest before it can emit
`RETAIL TRACE VERIFIED`, and supports explicit frame shifts for measured tick
alignment. Its synthetic self-test is always `NOT VERIFIED`.

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
