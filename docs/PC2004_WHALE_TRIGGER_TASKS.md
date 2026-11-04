# WCA60 — original AO summon/kill trigger contracts

CP6 is now IN PROGRESS at the source-rule/data boundary. There is no native
whale, scripted camera, pier destruction or chase acceptance yet. The prior
DOLPHIN object is a separate ambient actor; it is not substituted for the AO
sequence. No visual reference or native event replay was performed here.

The exact owner's registry entries67/68 point to4FBE40/4FBCE0. They install
4FBDA0/4FBC90 as main callbacks and initialize action0. Both use list3, flags2,
registry distance160000 and the original ClipSetObject/debug gates. The
441840 sphere service checks updated player collision centers, first player0
then player1, and returns ordinal1/2 rather than a mask. Radius is45, with a
strict source length comparison; the portable task consumes that service's
result instead of inventing an AABB or contact-center approximation.

Summon action0 arms only for ordinal1 (player0) and returns without spawning.
On a later update, action1 emits one summon unless the task is debug-selected;
it does not retest the sphere. The spawn position is SET position plus raw
parameters in world axes, with unchanged angleY. It then becomes action2.
4F88B0 first replaces an existing AO task, clears the shared kill request,
creates callback5029F0 and passes the resolved position/yaw to it. That
constructor installs main5022C0, display502210 and destructor5022A0. These
executor/constructor/display services are NOT implemented by the trigger helper.

Kill action0 accepts either ordinal1 or2 and calls4F8910 repeatedly while inside;
it does not advance its own action or delete the whale immediately.4F8910 only
sets the shared kill request if an AO task exists. Source clip/debug behavior
and task/SET lifetime remain caller services. Debug mode requests its separate
editor display and prevents ordinary task work; it is not a gameplay pause flag.

The owner converter retains four exact raw SET rows: act0 slots435/705 summon,
693/725 kill; act1 has none. It preserves all32 bytes and original slot/act
identity. The package has a versioned trigger chunk and witnessed radius/registry
distance; remaining clip fields are reserved0. It contains no whale art or code.
Private720B package SHA256
acb1f15800024d6b83721d5e6de6f666235772e443f005a6baafa7276a5c0f11
at /private/tmp/sa3ds-wca60-whale-triggers.sa3dspak is uninstalled and outside Git.

## Verification and next work

Portable checks PASS delayed summon, player0 selection, retained arming after
leaving the sphere, debug-selection deferral, terminal one-shot, both-player
kill and repeated request, clipping/debug gates and unrotated parameter sum.
Eight exact executable spans plus registry/call/immediate witnesses PASS;
converter checks the exact four slot identities and raw package round trip.
Required cumulative host, exact-owner pose/import gate and native build PASS.
The final reserved-field change was rechecked through the focused owner builder;
the unchanged pose matrix was not rerun. Native trigger helpers are unreferenced
and linker-discarded: native binary stays WCA59's470768B/013d0577 exactly. No
redundant emulator test ran; WCA58/59 lighting raster remains pending Mac unlock.

Next CP6 work: recover AO main5022C0, its external ACTION/model/motion hierarchy,
shared request/path state and camera/pier ordering; implement that executor and
its owner package consumer, then connect original trigger results to ordinary
world tasks. Do not spawn a decorative stand-in or mark native chase accepted
from these source-rule checks. Full WCA, retail and physical hardware gates open.

WCA61 continues the constructor/action dispatch in
[whale motion phases](PC2004_WHALE_MOTION_PHASES.md). It does not yet execute
these trigger requests in a native chase.
