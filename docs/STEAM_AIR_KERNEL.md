# Steam jump/air kernel — 0.0.8.5

This document records which parts of the Air Parity Candidate are direct
executable evidence and which remain compatibility assumptions.

## Direct Steam evidence

Supported clean Steam executable fingerprint:

- size `90,922,000`
- SHA-256 `e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6`

Key routines/locations:

- `0x004DDEC0` — Steam `PGetAccelerationAir` equivalent
- `0x004DB790` — analog query
- `0x004DBA90` — held-jump input/timer predicate
- `0x004E2480` — general turning helper
- `0x004E2950` — common speed integration/output path
- `0x00509819` / `0x0050981F` — normal jump reads `jmp_y_spd` and writes
  vertical local speed
- `0x00505ABC` / `0x00505AC2` — held-jump reads `jmp_addit` and multiplies by
  `0x00959FF0`
- `0x00959FF0` — double `0.800000011920929`

`0x004DBA90` checks control-disable status, verifies the mapped held-input mask,
increments the player's jump timer, compares the **old** timer to
`jump2_timer`, and returns true while the boost window remains valid.

## PGetAccelerationAir structure

The exact Steam routine:

- starts from gravity times `weight` plus a collision-force vector,
- transforms that vector into the player coordinate frame,
- adds `air_resist_air * Speed.x`,
  `air_resist_y * Speed.y`, and `air_resist_z * Speed.z`,
- reads analog angle and magnitude,
- uses `air_break * magnitude` for sufficiently high forward speed with an
  input-angle difference greater than `0x6000` BAMS (135°),
- supplies no air thrust above `0x1000` BAMS (22.5°) in the normal turn path,
- otherwise applies `air_accel * magnitude`, doubled when movement-direction Y
  is negative,
- calls the general turn helper, then
- writes the resulting local acceleration vector into the player's additive
  velocity.

## Current flat-lab specialization

`steam_air.c` implements the branch structure above with world gravity
`(0,-1,0)` and zero free-air collision force. SA3DS stores world velocity, so
it converts to/from a Sonic-local forward/vertical/side frame around yaw.

This is deliberately **not** the final slope/loop implementation. Surface
normal orientation, arbitrary gravity frames, wall/ceiling collision and
adhesion belong to 0.0.8.7.

## Remaining validation

Before claiming 1:1 air physics we still need original-SADX reference traces
for tap/held jump, rising/descending air control, reverse air input, and landing.
Those traces should include local velocity, world velocity, yaw and jump timer.
