# Portable PC2004 ground clamp and local-speed integration

`src/sa1/physics/pc2004_ground_accel.[ch]` reconstructs the ordinary force clamp
44C923..44CDE4 and local-speed part443F50..44412B against exact owner profile
SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
It contains gameplay arithmetic and source addresses, without private game
code/assets. `PC2004_GROUND_ACCELERATION.md` records the forward branch equations;
`PC2004_EXTERNAL_FORCE.md` explains the conditional static N=-weight context.

## Integration order

1. Honor the ordinary routine's delegated status/flag gates. Construct source
   world Gravity*weight plus external force and transform to the current task
   frame. Apply the source unstick/scaling branches and resistance **before**
   turning changes task angles or local speed. These steps remain caller-owned.
2. Select local input/brake force F and run source turning helpers. Preserve
   the separate source Up and surface multiplier/flag4 provenance.
3. Call `sa1_pc2004_ground_accel_clamp` with pre-turn force A/N/Z and
   **post-turn local speed**. Output is AddVelocity and animation residual.
   The source residual starts zero because the prior completed update cleared
   it; `wrote_residual` distinguishes the clamp's actual store from that zero.
   Never substitute motionwk2+34 for F on no-analog slowdown branches.
4. Call `sa1_pc2004_pgetspeed_local` with that post-turn local speed,
   AddVelocity, the integration-phase analog result, Up and player+1E4 auxiliary
   flags. Then perform source world transformation, pending displacement,
   velocity limits, contact/position resolution and reset-position separately.
5. Preserve the source tail phase that clears external force/residual and
   decrements the signed control timer. Do not decrement before analog queries.

`normal_addend` is the routine's separate normal output addition, ordinarily0.
The analog task status2000 branch sets it to-.8; it is added **after** friction
calculations, not folded into N. The primitive does not implement the preceding
special turn helper and does not independently authorize that branch.

## Side clamp and local-speed boundaries

The ground clamp's side zero-speed branch keeps the lim_frict*M*N threshold
retained for its strict interval test. With nonzero side speed, the stored
friction amount is grd_frict_z*M*N only when N<0, otherwise0. It opposes side
**acceleration** sign and prevents acceleration crossing through zero. Surface
flag4 selects a different instruction path; both paths are represented. The
low-Up no-analog multiply-by10 happens before this side clamp.

PGetSpeed's forward part integrates AddVelocity, prevents crossing the previous
forward sign, then snaps low speed with no analog and Up>.7. Its threshold is
H=float(jog_speed*.87). A previous forward speed exactly0 skips these nonzero
guards, even when the new value is small.

The side part is not a generic zero-cross clamp. It stores the new side speed
as float32. Positive old side speed below H snaps only if it is unchanged;
old side speed at/above H snaps when the new value is below H. Negative old
side speed mirrors those tests around -H. Old side speed0 skips these guards.
Consequently small positive side speed can cross zero when acceleration changes
it; replacing this with a generic axis no-cross helper changes source behavior.

After these steps, source player+1E4 bit400000 uses threshold jog_speed*.8;
otherwise bits400002 use (run_speed+jog_speed)*.5. Forward speed above
or equal to that retained threshold receives air_break. These auxiliary flags are distinct
from player+5C surface flags and task status; caller provenance remains explicit.

## Verification and limits

`tests/run_pc2004_ground_accel.sh` checks manually calculated synthetic branch
witnesses, strict threshold boundaries, flag4's asymmetric negative residual,
post-clamp normal addition, default-static first/subsequent acceleration and
source side-speed exceptions. `test_pc2004_ground_accel_evidence.py OWNER_EXE`
checks exact-profile range digests, relevant stores and float constants.

Retained double intermediates preserve the inspected store boundaries but do
not emulate all x87 extended precision. Finite inputs are required. The helpers
do not reconstruct world contact, moving-object scheduling, exceptional action
delegates or all pre-clamp force/input/turn selection. No native, retail-frame,
visual-pacing or hardware parity follows from these source checks.


## Shared integration in air and ball actions

PGetSpeed is not exclusive to ordinary ground movement. Exact source movement
entries for action8/12 reach44B9C0 at49B33E and443F50 at49B349; charge action4
reaches443F50 at49AE42. The native flat/general air and charge/Roll paths now use
the same recovered local integration rather than generic axis no-cross helpers.
Normal acceleration is integrated before the existing support adapter resolves
and resets grounded local normal speed. A sub-jog positive forward speed with
inactive analog and upright Up snaps in air too. Small side speeds that change
may cross zero; threshold crossings follow the original asymmetric side rules.
Auxiliary movement flags remain0 in these bounded ordinary contexts.

The action8 selector's held-jump call43BF90 is at4977CC and adds local
jmp_addit*.8 before movement. Action12 has no such selector branch, despite
sharing the same movement dispatch49B324. The public flat/general paths now
exclude source fall12 and spring9/10 from held-jump boosting. Legacy states
without exact action provenance preserve their existing helper behavior.

REA Ghidra12.1.4 independently decompiled exact-owner443F50 and496F50; source
instruction witnesses also retain the dispatch calls. This is static authority,
not a captured retail trajectory. Original PC2004 Emerald Coast was reached
through Moonlight and ordinary walking/running inspected, but no frame-aligned
retail trace or objective cycle equality was captured.
