# PC 2004 Break / Inertia / Collision Work Evidence

Authority: exact supported SADX 2004 US `sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

This document records only behavior supported by checked executable bytes.
Public reverse-engineering headers/projects are useful naming cross-checks, but
the exact executable remains the parity authority.

## PGetBreak — `0x00448E50`

Recovered field use:

- `playerwk+0x114` -> `weight`
- `playerwk+0xF0` -> `air_resist`
- `playerwk+0xF4` -> `air_resist_y`
- `playerwk+0xF8` -> `air_resist_z`
- `playerwk+0xE4` -> `run_break`
- `playerwk+0x104` -> `lim_frict`
- `playerwk+0xFC` -> `grd_frict`
- `playerwk+0x100` -> `grd_frict_z`
- `motionwk2+0x3C` -> acceleration multiplier
- `playerwk+0x2C` -> output AddVelocity

The checked supported path applies local gravity/collision force, drag, braking,
friction/deadband logic, axis zero-cross protection, then writes AddVelocity.

## PGetInertia — `0x00443650`

Recovered direct branches:

- `Speed.x > run_speed && Up < 0` -> multiply local **normal** force by `-8.0f`
- status `0x100` and `Up <= 0.98` -> subtract `Speed.x * 0.0002f`
- otherwise ordinary forward drag uses `air_resist * Speed.x`
- normal/side drag use `air_resist_y` / `air_resist_z`

Axis correction (2026-10-23): exact `0x4436F7..0x443701` loads, multiplies
and stores stack+0xC. Forward drag instead uses stack+8 at `0x4437B1`.
Normal drag reads Speed+0x3C and stack+0xC at `0x4437C3..CD`, then writes
AddVelocity+4 at `0x4437E0`; forward writes AddVelocity+0. PGetSpeed
`0x444031..37` adds AddVelocity+0x30 directly to local Speed+0x3C.
This operand/dataflow proof supersedes the older forward-force interpretation.
The input force is gravity*weight plus collision force, already transformed
world-to-player-local by `0x43EC00`; normal is contact-local Y, not world Y
on slopes/inverted frames. Pending dynamic impulses remain excluded below.
The portable helper now scales only normal before ordinary normal drag.
The source-equation regression uses 98 signed-speed/Up/status cases, including
strict run-speed and zero-Up boundaries. It fails on the former source.
These synthetic inverted frames do not establish retail inverted-contact parity.
An additional120-tick synthetic inverted basis checks direct normal-speed
integration against the closed-form drag recurrence and finite world position;
the existing Emerald Coast upright-support regressions remain the real-geometry
guard. The resident opening floor adapter does not support inverted contact.

A grounded branch consumes and clears data through the separate
`playerwk+0x64` collision/support work block. 0.0.8.8.8 intentionally models the
zero-pending-impulse specialization for the static lab instead of guessing that
block's complete semantics.

## PGetSpeed — `0x00443F50`

Direct evidence shows AddVelocity being integrated into Speed and an explicit
forward zero-cross guard. A `0.87f * jog_speed` low-speed threshold is also
present. Only the zero-cross invariant is promoted into the portable kernel in
this checkpoint; the remaining action/status branch semantics stay unresolved.

## Collision/support work

`PInitialize` (`0x00442750`) uses a 0x84-byte stride for character physics
records, copies 33 dwords (`0x84` bytes) into `playerwk+0x9C`, and separately
allocates another `0x84`-byte block whose pointer is stored at `playerwk+0x64`.
It also writes the ordinary `1.0f` acceleration multiplier to `motionwk2+0x3C`.

`PSetPosition` (`0x0044CDF0`) first:

- copies `SurfaceFlags` to `SurfaceFlags_Old`
- clears current SurfaceFlags and scratch contact-vector fields
- dispatches into one of two substantial collision helper paths

Both checked helpers read the separate `playerwk+0x64` work block and load
`playerwk.height`, multiplying it by exact `0.5f` during collision-work setup.
This proves that the retail collision resolver is structured state, not the
single support ray previously used by the white-lab prototype.

`PResetPosition` (`0x0043EE70`) writes the post-collision local speed back into
`playerwk+0x38` and updates the local support frame / `Up` state.

## Explicit non-claims

This checkpoint does **not** yet assign neutral field names to every member of
the 0x84 collision block. It also does not claim exact floor/wall/ceiling probe
positions, probe count, helper-level query order, dynamic-object impulse
semantics, or support arbitration. Those are the target of the next focused
collision reconstruction pass.

## Reproducible evidence gate

Run with the exact owner-supplied executable:

```sh
python3 tools/verify_pc2004_break_inertia_collision.py /path/to/sonic.exe
```

The verifier fingerprints the executable, checks the relevant instruction/data
signatures and constants, and never writes Sega executable bytes into the
checkpoint.
