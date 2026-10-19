# Steam SADX physics evidence — through 0.0.8.8

Supported executable:

- size: `90,922,000`
- SHA-256: `e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6`
- PE32 image base: `0x00400000`

## Confirmed Steam player parameter table

The exact Steam executable contains an eight-entry table at:

- VA `0x025D9B90`
- record stride `0x84`
- 8 records

This is no longer just a float-pattern guess. Steam function `0x004DAA00` performs:

1. read player character ID byte at offset `+0x09`,
2. multiply it by `0x84`,
3. add `0x025D9B90`,
4. copy `0x21` dwords = `0x84` bytes into the runtime player work structure.

The public SADX character enum orders IDs as Sonic, Eggman, Tails, Knuckles,
Tikal, Amy, Gamma, Big, so table entry 0 is Sonic.

`tools/extract_steam_player_params.py` independently verifies the exact EXE
fingerprint and these initializer instruction signatures before emitting data.

## Confirmed Steam PGetAcceleration-equivalent

Steam routine start: `0x004DBE50`.

It has the same distinctive high-level control structure as the publicly
researched 2004 SADX `PGetAcceleration` routine:

- DisableControl status path (`0x4000`)
- Tube surface flag path (`0x20000`)
- Stair surface flag path (`0x4000`)
- weight-based gravity/surface contributions
- unstick thresholds involving Up and speed
- jog/run/rush/dash thresholds
- `run_accel`, `slow_down`, `air_resist*` parameter use
- 22.5-degree (`0x1000` BAMS) and 45-degree (`0x2000` BAMS) steering thresholds

Several distinctive constants also match the public 2004 cleaned
decompilation exactly, including approximately:

`0.1`, `0.6`, `1.16`, `-0.4`, `5.0`, `-0.3`, `0.8`, `-0.1`, `0.4`,
`0.5`, `0.7`, `0.87`, `4.225`, and steering magnitude factors.

This is strong evidence that the core acceleration algorithm was carried
forward from the 2004 PC codebase into Steam.

It is **not** evidence that every PC/Steam/Dreamcast physics subsystem,
collision behavior, frame cadence, input processing, or bug is identical.
Those still require per-system verification.

## Sonic Steam parameter evidence

Entry 0 currently decodes as:

- jump2_timer 60
- max_x_spd 3.0
- max_psh_spd 0.6
- jmp_y_spd 1.66
- jog_speed 0.46
- run_speed 1.39
- rush_speed 2.30
- crash_speed 3.70
- dash_speed 5.09
- jmp_addit 0.076
- run_accel 0.05
- air_accel 0.031
- slow_down -0.06
- run_break -0.18
- air_break -0.17
- air_resist_air -0.028
- air_resist -0.008
- air_resist_y -0.01
- air_resist_z -0.4
- grd_frict -0.1
- grd_frict_z -0.6
- lim_frict -0.2825
- rat_bound 0.3
- rad 4.0
- height 10.0
- weight 0.08
- eyes_height 7.0
- center_height 5.4

These values are stored as *evidence* in `steam_player_parameters.c`; they are
not dropped into the old Movement Lab equation, because doing so would misuse
them. The next parity step is to port/validate the actual equations that
consume them.


## Confirmed Steam air/jump evidence

- `PGetAccelerationAir` equivalent: `0x004DDEC0`
- held-jump predicate/timer helper: `0x004DBA90`
- normal Sonic jump reads `jmp_y_spd` at `0x00509819` and writes local
  vertical speed at `0x0050981F`
- held-jump state reads `jmp_addit` at `0x00505ABC`
- multiplier constant at `0x00959FF0` decodes as double
  `0.800000011920929`

The Steam air routine directly consumes `weight`, `air_resist_air`,
`air_resist_y`, `air_resist_z`, `air_accel`, `air_break`, `run_speed`, and the
SADX analog angle/magnitude. It has explicit 22.5-degree and 135-degree input
angle thresholds.

These findings justify replacing the old Movement Lab jump-cut/air formula,
but they do not by themselves certify all landing, surface-relative, or
PGetSpeed behavior as frame-identical.

## 0.0.8.6 ball-state boundary

The public SADX player-work structures establish dedicated Spin Dash speed
storage and a ball status bit. The exact supported-Steam Sonic-specific charge
handler is not yet identified strongly enough to assign its charge constants.
The current `2.0 / 10.0 / 0.4` candidate is therefore isolated and explicitly
labeled HYPOTHESIS; it must be replaced if original-game traces or stronger
Steam disassembly disagree.

## 0.0.8.7 general-surface architecture boundary

Public SADX tooling exposes player-work surface-normal state and geometry
movement helpers that move position/velocity/angle against arbitrary geometry.
That is enough to reject the old SA3DS `ground_y(x,z)` assumption as a final
architecture. It is **not** enough to claim the exact retail adhesion and
non-flat friction equations. 0.0.8.7 therefore ports the architecture first
and keeps its unresolved thresholds explicitly marked candidate.


## 0.0.8.8 exact Steam steep-surface evidence pass

The exact locked Steam executable was independently re-fingerprinted and the
`PGetAcceleration` candidate at `0x004DBE50` was disassembled directly.
`tools/verify_steam_surface_branches.py` now turns a selected portion of that
research into a reproducible evidence gate: it refuses the wrong executable,
then checks 14 instruction signatures and 14 decoded double constants.

The verified branch constants include approximately:

- Up/surface-orientation thresholds: `0.1`, `-0.1`, `-0.3`, `-0.4`, `0.5`, `0.7`, `0.87`
- speed threshold: `1.16`
- force factors: `0.4`, `0.8`, `1.4`, `4.225`, `5.0`
- cross/unstick comparison factor: `0.6`

The corresponding recovered branch shape is isolated in
`steam_surface_evidence.c`, including the low-speed steep-surface force
scaling and the inverted/steep normal-bias multipliers. These values are no
longer hand-tuned Movement Lab guesses.

One boundary remains important: the runtime's `normal_y < 0.3` and tangent
speed `< 1.16` support-fall rule is backed by public SA1 collision
reconstruction, but has not yet been independently located in the supported
Steam collision routine. Likewise, exact retail support probes, collision
query ordering, orientation update ordering, and the point where the recovered
normal bias causes actual support loss remain unresolved.
