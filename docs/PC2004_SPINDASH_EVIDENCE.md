# SADX 2004 Sonic Spin Dash evidence — 0.0.8.8.6

This note records static evidence recovered directly from the exact supported
SADX 2004 US `sonic.exe` profile:

- size `55,549,952`
- SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`

No executable bytes are included in the SA3DS checkpoint.

## Sonic action dispatch

The Sonic main action dispatch at `0x0049AB1D` indexes the table at
`0x0049BFBC`.

- action `4` -> `0x0049AE2F` — Spin Dash
- action `5` -> `0x0049AEBB` — Roll

The public Sonic action enum independently names the same action IDs.

## Exact Spin Dash entry

The entry routine beginning at `0x00496EE0` proves the ordinary Spin Dash
transition:

- writes action `4`
- enables the ball/attack status bits
- resets `SonicSpinTimer` at player-work `+0x7E`
- compares local forward speed `+0x38` with the f32 at `0x007DCB94`
- that f32 is exactly `2.0f`
- if forward speed is at least `2.0f`, it copies that speed to
  `SpindashSpeed` at player-work `+0x00`
- otherwise it writes immediate `2.0f`

There is **no 10.0 clamp on entry**. A moving Sonic can therefore begin a
charge above 10.0.

## Exact charge / timeout / release

The transition routine at `0x00495080` proves:

- the ordinary timeout comparison is `300` frames (`0x12C`)
- while the charge input is held, `SpindashSpeed` is compared with the f32 at
  `0x007DCB34`, which is exactly `10.0f`
- if charge is below `10.0f`, it adds the f32 at `0x007DD550`, which is
  `0.4000000059604645f` (the f32 representation of 0.4)
- the add is followed by a store; there is **no post-add clamp**

That last detail matters. Starting from an arbitrary moving value such as 9.9
can produce about 10.3. Even standing charge can cross the nominal 10.0 compare
threshold because repeated f32 `+0.4` storage can leave the prior value just
below 10.0; the next accepted increment produces about 10.4.

On ordinary release the routine:

- writes action `5` (Roll)
- copies `SpindashSpeed` to local forward `Speed.x`
- resets `SonicSpinTimer`
- clears `SpindashSpeed`

At the 300-frame timeout it leaves charge state and resets the timer. The
observed timeout path does not clear `SpindashSpeed`; SA3DS preserves that stale
internal value, although it is overwritten on the next Spin Dash entry.

## Transition, timer, and movement order (corrected 2026-10-22)

The earlier note incorrectly placed the timer before the transition handler.
Direct disassembly of the exact executable shows:

1. `0x0049AA34` calls `Sonic_Act1` at `0x00496F50`.
2. Action 4 transitions call `0x00495080`; action 5 calls the roll-end
   helper `0x004930D0` at `0x00497458`.
3. `0x0049AAF2..0x0049AB01` tests ball status and increments `+0x7E`.
4. `0x0049AB29` dispatches movement using the resulting action.

Entry therefore resets the timer and finishes its first movement tick at 1.
Held transitions inspect the previous timer, then increment after the check.
Release writes forward speed from charge and clears the timer/charge, then
finishes at timer 1 with action-5 inertia already applied. The post-tick speed
can differ from the charge by resistance and projected slope gravity. Timeout
checks the previous timer >=300 and dispatches ordinary movement that tick.

`0x004930D0` compares signed forward speed (`+0x38`) against run_speed
(`+0xC4`) before movement. Speed below run_speed exits Roll; absolute magnitude
is not tested. Equality remains rolling through this tick's inertia. The
helper selects retail action 1 at nonpositive speed or action 2 otherwise and
requests animation 29. The port's neutral GROUNDED action combines 1/2;
dedicated Uncurl request propagation remains unresolved.

Spin Dash entry calls `0x00495C30` at `0x00496EFB`. Both ordinary and alternate
presentation branches request animation 14. Its exact metadata property 10
uses the live SpindashSpeed scalar times 0.2, rather than fixed-rate row 32.

## Exact action movement call order

### Jump arbitration (2026-10-22 continuation)

Exact-PC2004 disassembly and the saved private Ghidra export agree:
Sonic_Act1 action 1 calls jump helper `0x495E60` at `0x49707F`, before
charge entry at `0x4970B0`; action 2 does likewise at `0x4971B6` before
`0x4971E7`. Action 5 calls jump at `0x497446` before roll termination at
`0x497458`. Action 4 instead reaches charge logic at `0x4973D7` without
a jump test. Thus a jump pressed while charging (including its release tick)
does not launch; a later press from Roll does. Normal simultaneous jump/action
input prioritizes jump.

The ordinary helper writes action 8 and local Y from jmp_y_spd at
`0x495F0F..0x495F18`, requests 14 via `0x495F43`, and resets the jump timer
at `0x495F63`. Main then dispatches movement by that resulting action.
SA3DS no longer runs ground movement before a normal launch, and the surface
path now skips held-jump boost on the initiation tick like the flat path.
The airborne-state hold helper begins on subsequent ticks. This establishes
operation ordering, not complete slope-air or retail runtime numerical parity.

Regressions cover eight flat launch/input-priority cases and 36 launches on
six actual imported support locations (normal/roll × neutral/forward/turn),
plus held-charge jump immunity. The old HEAD source fails the new launch test;
the corrected source passes. Exact call/impulse/timer signatures are checked
by `tools/verify_pc2004_spindash.py`.

Action 4 at `0x0049AE2F` calls:

`PGetRotation -> PGetBreak -> PGetSpeed -> PSetPosition -> PResetPosition`

Action 5 at `0x0049AEBB` / shared continuation `0x0049BBE2` calls:

`PGetRotation -> PGetInertia -> PGetSpeed -> PSetPosition -> PResetPosition`

This is useful evidence for the next roll/surface parity pass. 0.0.8.8.6 does
not claim that every collision side effect inside `PGetBreak`, `PGetInertia`,
`PSetPosition`, and `PResetPosition` has already been reimplemented.

## Reproducible verifier

Run:

```sh
python3 tools/verify_pc2004_spindash.py /path/to/sonic.exe
```

`--json` emits a machine-readable evidence report.
