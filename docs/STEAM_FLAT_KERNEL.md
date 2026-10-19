# Steam flat-ground kernel — 0.0.8.4

## Scope

This kernel is intentionally restricted to grounded surfaces whose slope is
near zero. It reconstructs the supported Steam SADX ground-input path without
claiming that the still-placeholder slope/collision system is solved.

## Input shaping

The Steam routine at `0x004C3FC0` reads signed controller axes, shifts them by
8, strips a **per-component ±3072 deadzone**, computes direction with `atan2`,
and computes magnitude as:

```text
r2 = x*x + z*z
magnitude = sqrt(r2) * r2 / (29441^3)
magnitude = min(magnitude, 1)
```

So the response is cubic after deadzone removal. `steam_input.c` reproduces
that curve after first mapping the device-independent `-1..1` axes onto the
signed 8-bit scale used by the PC peripheral path.

The 3DS adapter itself no longer adds a second radial deadzone; it only maps the
Circle Pad's practical range onto `-1..1`.

## Ground movement

The kernel uses the validated Sonic Steam parameter record and reconstructs the
flat/upright branches of `PGetAcceleration` at `0x004DBE50`, including:

- `run_accel`, `slow_down`, and `air_resist`
- jog/run/rush/max-speed thresholds
- the 0.5 / 0.7 / 0.9 analog-magnitude branches
- 22.5° (`0x1000`) and 45° (`0x2000`) steering thresholds
- high-angle braking instead of immediate reverse thrust
- no-input slowdown

For supported flat ground, the floor contact reaction cancels gravity's local-Y
contribution before the friction tail. This is the specialization used by the
white-space runway; non-flat contact forces are deliberately deferred.

## Turning

Steam turning helpers independently identified in the supported EXE:

- `0x004E2480` — general helper
  - delta >45°: max step `0x800`
  - 22.5°..45°: delta / 4
  - <=22.5°: delta / 8
- `0x004E2770` — quick helper, maximum `0x2000` (45°)
- `0x004E2800` — slow/high-speed helper
  - starts at `0x100`
  - above `dash_speed`, step is reduced by
    `round(sqrt((speed-dash_speed)/16) * 256)`

On flat upright analog movement, the general/slow helpers preserve almost all
world inertia through the verified 0.99 / 0.01 local-speed blend. The quick
helper restores the old local vector after changing yaw.

## Integration

`PGetAcceleration` produces player-local acceleration. Steam routine
`0x004E2950` then adds that acceleration to local speed and prevents forward
speed from crossing through zero. With no analog input it also performs an
upright low-speed stop using `jog_speed * 0.87`.

0.0.8.4 now follows that separation instead of directly treating the selected
force as final world velocity.

## Still unresolved

- frame-by-frame original SADX trace validation
- exact behavior of every collision-force/friction branch away from level flat
  ground
- jump / held jump / release / air control
- roll and Spin Dash state equations
- general triangle collision, steep-surface adhesion, walls and loops
- Dreamcast-vs-Steam behavioral differences

Therefore this checkpoint is a **Ground Parity Candidate**, not a final 1:1
certification.
