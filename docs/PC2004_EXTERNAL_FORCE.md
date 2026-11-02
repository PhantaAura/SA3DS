# PC2004 external force and static support

Static inspection uses owner PC2004 executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`,
LLVM disassembly and the retained private decompilation. Those private inputs
remain outside Git. This report establishes a bounded ordinary static context;
it is not a retail memory trace or a claim about all object/action producers.

## Three different vectors

The ordinary acceleration entry44C2E0 constructs world force as
Gravity*weight plus **motionwk2/EntityData2+10,+14,+18** and transforms it at
44C357. This vector must not be confused with world velocity at motionwk2+4,+8,+C,
or with the separately allocated collision scratch reached through player+64.
That scratch has velocity at+10,+14,+18 and response velocity at+6C,+70,+74.
Identical offsets on those different allocations do not establish a normal
reaction in the acceleration input.

The completed player tail446F10 clears external force at446F2A..446F30.
It also clears player AddVelocity and animation residual. Setter441750 writes
the three external-force components of the selected player's motionwk2 directly.
These values are therefore transient; a producer between tail and acceleration
must supply any nonzero contribution for the next acceleration calculation.

## Ordinary terrain contact

PSetPosition44CDF0 calls the ordinary terrain resolver449500. That resolver
copies motionwk2 world velocity into the separate collision scratch before
testing geometry. Its helpers43CCE0/43D360 adjust scratch response and sometimes
motionwk2 world velocity. Response helper444AA0 copies scratch+6C/+70/+74 back
to scratch+10/+14/+18 and adjusts task position. These stores target collision
scratch, not motionwk2 external force. Helper43DCA0's explicit global-player
lookup can also replace motionwk2 world velocity; its stores are+4/+8/+C.

PResetPosition43EE70 transforms world velocity into local speed. Grounded status
low bits cause a **local normal speed zero** at43EED6; the resulting local
speed is written to player+38/+3C/+40. Its other stores clear angle high halves
and update the source Up scalar. It does not supply external normal reaction.
The completed tail subsequently clears motionwk2 external force regardless.

For an ordinary upright static update with no intervening external/special
producer, zero pending collision impulse, local normal speed0 and unchanged
default gravity, the acceleration input is consequently N=-weight, not N=0.
Static contact alone is not evidence for adding +weight to external force.
This is a conditional source context; reproducing a complete retail task
schedule or an arbitrary contacted object still requires its own evidence.

With default Sonic parameters and ordinary M=1, N=-.08 gives the positive
forward limit K=lim_frict*M*N approximately .0226. After forward speed becomes
positive, analog F=.05 and A=0 enter the positive-force clamp: applied forward
acceleration is approximately .0226 and residual approximately .0274. The exact
first tick at speed0 follows the separate zero-speed branch and does not write
that residual. This explains why declaring flat animation residual universally
zero can change pacing even before slope support is considered. Float/x87
rounding and the source steering/clamp order remain relevant.

## Producers that prevent a universal zero-force assumption

The Sonic jump selector495E60 reads a nonnull support task through player+6C,
then that task's motionwork at task+28, and passes its+10/+14/+18 vector to
441750. Other character and board branches use the same pattern. This is
explicit support-object coupling; no support task means that branch supplies
no vector. It must not be generalized to static terrain triangles.

Special routine448000 directly writes external Y=weight*.95 before calculating
its own acceleration. Target-guided routine44BD70 may add horizontal external
force after predicting a trajectory toward a registered target, then calls
443650. Other callers of441750 provide character-specific and scripted forces,
including water-related movement. These paths prove the field is not reserved
exclusively for moving platforms. Their action gates and complete scheduling
are not reconstructed here, so a blanket zero field for all grounded or
airborne actions is unsupported.

The collision pending impulse buffer reached through player+64 at+78/+7C/+80
is another distinct input. Routines4432E0/443650 consume and clear it; the spring
no-input helper explicitly requires that input to be zero. This report does
not prove that all static terrain or all dynamic CCL interactions leave that
buffer zero. Keep that assumption visible rather than folding it into gravity.

## Verification

`tests/test_pc2004_spring_runtime_evidence.py OWNER_EXE` verifies exact-profile
digests for the external setter, completed tail, reset-position, ordinary land
resolver and acceleration force entry, plus short witnesses for the tail clear
and grounded local-normal-speed clear. A digest preserves the inspected owner
range identity; it is not an automated proof of every negative dataflow claim.
No original instruction bodies, original assets or decompilation are included.
