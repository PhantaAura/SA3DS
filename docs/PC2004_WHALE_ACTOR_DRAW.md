# WCA66 — whale actor update order and original cycle drawing transforms

`whale_actor.[ch]` connects the recovered constructor state, action phases,
collision-tip sampler and common tail. Shared tip is supplied explicitly on
construction because the source stage value survives AO replacement. The second collision shape
starts disabled by attribute0x10; emergence clears that flag. Phase requests receive the PREVIOUS tip,
then the new tip is sampled BEFORE horizontal movement. Bounce/collision/wave/
display retain the WCA62 service order. The caller still executes the actual
phase effects, deferred deletion and native stage services.

A dive deletion request still executes the common/display tail. The separate
far-distance deletion stops it immediately. Phase-effects callbacks therefore
must mark a deferred deletion, not free actor storage while the tail is running.
Constructor camera-disable/CCI setup, shared AO lifetime and original task-list
ordering still belong to the future native executor; the portable initializer
only establishes actor state. These interfaces do not silently emulate missing
native services with current player position or invented collision responses.

`whale_draw.[ch]` recovers the cycle's skeletal root pitch separately from the
collision tip.404C10 uses denominator1 when the next frame difference is not
positive; the cycle32-key last->first segment blends normally. This differs
from both the collision tip's custom29->0 selection and the shape channel's
unsigned huge last-key denominator. Raw signed integer angle blending truncates
toward zero. Owner Ninja sine is a caller service, not replaced with libm in
gameplay. The display transform follows502210 translation(baseY+drawY), yawY,
rollZ and sampled rootX, preserving the axis helper float-store boundaries.
Caller selects the exit/cycle shape bank by action2 versus other actions.

Exit's separate root-motion descriptor points to the PE zero-filled tail at
03C5E3F8. A whole-file literal-address scan found only its motion-header pointer,
with no literal pointers to the other three words. This establishes initial
zero-fill and a bounded static lead, not a proof excluding all indirect runtime
writes. The draw helper therefore accepts caller-resolved root pitch; no exit
identity fallback or original-game runtime acceptance is claimed here.

## Checks and limits

Required cumulative host, exact-owner pose/import gate and native build PASS.
Synthetic integration checks cover ten-update emergence/CCL enable, retained old
tip for splash/rumble requests, first-wave clock, phase-before-common ordering,
dive versus far deletion, signed root-angle truncation and Y/Z/X transform order.
The owner pitch audit checks all32 raw words and257 cycle root samples at eighth
frames against separately addressed owner rows; eight exact code/data spans
constrain the new helpers. Existing shape/visual/pose checks also pass. Double
intermediates approximate bounded x87 expressions; all extended-precision word
parity and retail trajectories remain unproven.

The actor/draw helpers remain unreferenced by native gameplay and linker-discarded.
Artifact remains470768B SHA256
013d05779a47d8d3171e8e3e9ea737356fa48e60808ea7fdc0dda8d9948be642;
no duplicate emulator replay. No owner package format/data changed, and shared
SD/inspected WCA56 Desktop preview remain unchanged. The artifact's optional
Sonic lighting is still raster-rejected/inactive.

Next: resolve exit root's null-channel/bind behavior, connect native loader,
trigger/task lifetime, player history/CCL and phase effects to the real actor,
then inspect natural spawn/animation and implement original pier/camera. Full
CP6 and CP1–CP9 remain incomplete; no hardware freeze-fix claim.

WCA74 update: the stock initial exit descriptor and404E26 bind-angle fallback
are now explicitly supported by a versioned owner profile. See
[exit root](PC2004_WHALE_EXIT_ROOT.md). Native actor/draw binding and live guest
mutation evidence remain separate.
