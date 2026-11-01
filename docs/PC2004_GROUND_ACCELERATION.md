# PC2004 ground acceleration and animation residual

This is a source-only reconstruction of the ordinary `PGetAcceleration`
routine at `0x0044C270`. It records equations and instruction witnesses, not
original source code, game assets, or a retail execution trace. No gameplay
implementation or parity acceptance follows from this report.

The exact owner executable examined has SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Evidence came from that executable's LLVM disassembly and the owner's private
decompilation. The executable and decompilation remain outside the repository.

## Fields and notation

| Symbol | Source field or meaning |
| --- | --- |
| `v` | `CharObj2+0x38`, signed local forward speed at the clamp |
| `u` | `CharObj2+0x1C`, source Up scalar |
| `q` | Result of `GetAnalog 0x0043BDC0`, sampled before steering |
| `A` | Local forward gravity/collision acceleration plus resistance, before input force |
| `N` | Local normal gravity/collision acceleration, including selected unstick bias and normal resistance |
| `Z` | Local side gravity/collision acceleration plus side resistance |
| `F` | Selected forward input/brake force, local stack variable at the clamp |
| `M` | `EntityData2+0x3C`, surface friction multiplier |
| `K` | `lim_frict * M * N`; `lim_frict` is `CharObj2+0x104` |
| `B` | `grd_frict * M * N`; `grd_frict` is `CharObj2+0xFC` |
| `R` | `EntityData2+0x38`, forward-force residual used by animation |
| `flag4` | Bit `0x4` of `CharObj2+0x5C`, distinct from task status |

`F` is not universally the contents of `EntityData2+0x34`. The analog branch
stores its selected force there. The no-analog branch stores zero there even
when its local `F` is a nonzero signed slowdown force. Under ordinary world
gravity and a corresponding support frame, `u` can be related to support-normal
Y; that relationship must not replace the source field in unsupported contexts.

The comparisons below are for finite, ordered operands. Constants such as
`0.051` refer to the executable's float constants. Strict and inclusive bounds
are intentional.

## Entry gates and force construction

The routine delegates before ordinary force construction when task status
`0x4000` is set (`0x00447510`), when player flags contain `0x20000`
(`0x0044B0E0`), or when player flags contain `0x4000` (`0x00447010`). These entry
gates occupy `0x0044C270..0x0044C2DF`; the ordinary equations must not be claimed
for those paths.

At `0x0044C2E0`, construct the world force:

```
worldForce = Gravity * weight + EntityData2[0x10, 0x14, 0x18]
```

`weight` is `CharObj2+0x114`. The call to `0x0043EC00` at `0x0044C357`
transforms this vector into the current task-local frame. Gravity/collision
projection and resistance happen before the steering helpers change orientation
and local speed. The later clamp reads the resulting post-steering `v`.
Projecting gravity only after steering changes this ordering.

The primary unstick condition is `u < 0.1`, absolute source cross-product Y
greater than `0.6`, and forward speed greater than `1.16`. It replaces the local
force with `(0, -weight, 0)`. Otherwise, with speed greater than `1.16`, the
successive `u < -0.4`, `u < -0.3`, and `u < -0.1` branches subtract respectively
`5*weight`, `0.8*weight`, or `0.4*weight` from normal force.

If none of those normal-bias branches is selected, the tangent scaling is:

| Condition, in order | Effect before resistance |
| --- | --- |
| `u < 0.5` and `-run_speed < v < run_speed` | Forward and side force multiplied by `4.225` |
| Otherwise, `u < 0.7` and `-run_speed < v < run_speed` | Side force multiplied by `2` |
| Otherwise, `u < 0.87` and `-run_speed < v < jog_speed` | Side force multiplied by `1.4` |

The last interval is signed and asymmetric; it is not `abs(v) < jog_speed`.
These decisions use the speed available before steering.

Forward resistance is then added. With analog input, speed above `max_x_spd`
and `u > 0.96` uses `(v-max_x_spd)*air_resist*1.7`; otherwise speed above that
maximum uses `(v-max_x_spd)*air_resist`, and negative speed uses
`v*air_resist`. Without analog input, speed above `run_speed` or negative speed
uses `v*air_resist`; the intervening source maximum-speed branch remains part of
the original ordering. Normal resistance adds `air_resist_y*Speed.y` to `N`;
side resistance adds `air_resist_z*Speed.z` to `Z`.

The routine then selects `F` and runs steering helpers. For no input and
`u >= 0.71`, positive speed selects `slow_down`, negative speed selects
`-slow_down`, and zero speed leaves `F=0`. Lower-Up no-input handling calls
`0x004491E0` and is a separate context dependency. Task status `0x2000` and
player flags `0x800000` also select additional input/steering behavior; treating
these paths as ordinary logical-stick input is not justified here.

## Forward clamp order

`R` is normally zero on entry because the previous completed Sonic update
clears `EntityData2+0x38` at `0x00446F45`. The acceleration routine does not
unconditionally clear it itself. A new standalone primitive should initialize
its own returned residual to zero rather than inheriting stale caller memory.

First, at `0x0044C923..0x0044C977`, if `u < 0.71`,
`-jog_speed < v < jog_speed`, and `q == 0`, multiply `A` and `Z` by ten and
bypass the remaining forward clamp. Applied forward acceleration is `10*A`;
`F` is not added and no residual is written.

Second, if `v == 0`, compute `V=A+F`. With no analog input, zero the result if
either `-K < V < K` or `-0.051 < V < 0.051`. Otherwise apply `V`. This branch
does not write `R` (`0x0044C97C..0x0044CA03`).

For the remaining branches, the common fallback is:

```
if q == 0 and v <= jog_speed and -0.051 < A < 0.051:
    applied = 0
else:
    applied = A + F
```

For `v != 0` and `F >= 0`, use this order:

1. If `v < 0` and `-0.051 < A < 0.051`, compute `V=A+F`; zero it when
   `V*F < 0`, otherwise apply it. Do not write a residual.
2. Otherwise, if `K > 0` and `F > K`, compute `V=A+F`. If `V > K`, write
   `R=V-K` and apply `K`; otherwise apply `V` without a residual.
3. Otherwise use the common fallback.

For `F < 0` with `flag4` clear:

1. If `v > 0`, compute `V=A+F`; zero it when `V*F < 0`, otherwise apply it.
2. For `v <= 0`, the common no-input deadband condition produces zero.
   Otherwise compute `V=K+F`, not `A+F`; zero a nonnegative `V`, otherwise
   apply it.
3. This path does not write a residual (`0x0044CAC2..0x0044CB5E`).

For `F < 0` with `flag4` set:

1. Only when `N < 0`, `B > 0`, and `-F > B`, enter the special clamp and
   compute `V=A+F`.
2. If `V < -B`, write **`R=V-B`** and apply `-B`. The subtraction is literal
   source behavior; do not replace it with the more intuitive `V+B`.
3. If `V > B`, write `R=V-B` and apply `B`.
4. Otherwise apply `V` without a residual.
5. When the special-clamp constraints are not met, use the common fallback.

This describes forward acceleration and residual only. Side friction, normal
output, speed integration, zero-cross/deadband handling in `PGetSpeed`, contact
resolution, and animation selection remain subsequent or separate operations.

## Surface multiplier and flag provenance

`0x0043CB90` ORs incoming flags in `EBX` into `CharObj2+0x5C`. Its later
`0x0043CCA4..0x0043CCDE` sequence derives `M` from those incoming flags in this
priority order:

| Incoming flag | Multiplier |
| --- | --- |
| `0x08` | `0.15` |
| Otherwise `0x04` | `0.1` |
| Otherwise `0x10` | `0.5` |
| Otherwise `0x80` | `1.5` |
| Otherwise | `1.0` |

Player initialization also stores `M=1.0` at `0x00442949`. The setter proves the
relationship between flags, the clamp's `flag4`, and the multiplier. It does not
by itself prove that an arbitrary reconstructed collision contact supplied the
same flags or was processed at the same phase. No terrain names are inferred
from these bits.

## Exact byte witnesses

All addresses are virtual addresses in the executable identified above. These
are short instruction witnesses, not copied routine bodies.

| Address | Bytes | Meaning |
| --- | --- | --- |
| `0x00446F45` | `897838` | Completed-update scratch clear |
| `0x0043CB90` | `09585c` | OR incoming surface flags into player flags |
| `0x0044CA1D` | `844d5c0f849c000000` | Surface flag4 branch |
| `0x0044CB63` | `d98504010000d84b3cd84c2420d95c2444` | `lim_frict*M*N`, stored limit |
| `0x0044CA3B` | `d985fc000000d84b3cd84c2420d9542444` | `grd_frict*M*N`, stored limit |
| `0x0044CC07` | `d8642444d95b38d9442444eb44` | Positive residual subtraction/store, then load applied limit |
| `0x0044CA9C` | `d8642444d95b38d9442414e9ac010000` | Negative residual subtraction/store, then load negative applied limit |

## Minimum faithful next step and limitations

A pure clamp primitive can accept explicit `A`, `N`, `F`, post-steering `v`,
`u`, `q`, `M`, `flag4`, and the original parameters, and return applied forward
acceleration plus a zero-initialized residual. It should preserve the branch
ordering above, including asymmetric negative-force behavior. Integration should
be restricted to contexts with backed force, friction, flag, and timing inputs.

The general-surface path lacks complete source collision-force-buffer and
normal-force provenance. The grounded-steering correction after `d1abe82` now
captures gravity and side resistance before steering; complete normal-force,
unstick and clamp integration remains open. Copying input force into the
residual, or declaring zero residual valid for all
surfaces, would not implement the source clamp. Gravity alone can yield nonzero
`N` and a positive friction limit: supported contact must not be assumed to
cancel it without evidence for the reaction-force buffer and update order.

Candidate acceleration can remain on the x87 stack through comparisons and the
residual subtraction. The byte sequences specify the stores; float-only C
reassociation or an extra candidate store is not an exact precision model.
Finite comparison behavior, native integration, independent original-game
context capture, retail frame traces, dynamic-object forces, exceptional player
flags, and hardware behavior remain pending. This report does not validate any
of those checks.
