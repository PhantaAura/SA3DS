# WCA48 — original checkpoint player/box contact

WCA47's asset pipeline retained source form3, but its rules builder mistakenly
selected enemy category3. Source44F5C7 passes category4 to CCL_Init41CAF0.
This checkpoint fixes that mismatch and upgrades the package/reader to version2;
version1 is rejected. The independent owner oracle now reads the initializer's
category immediate and compares the decoded runtime rules to the owner's table.
The earlier private WCA47 package is superseded and must not be installed.

The new portable contact path follows41FD00 primary7/other3 ->419C90 and its
4199D0/4550A0 rectangle helper. The checkpoint CCI is a thin box:12/4/.5
half-dimensions, local center0/6/1. It is not the enemy sphere contact path.
Default419270 center evaluation uses task T/Rz/Rx/Ry; rectangle dimensions use
only combined task/CCI Y rotation, as the original helper does. Box X/Z tilt
must not be substituted into the rectangle test merely because it looks natural.

419C90 reads player task-position history selector1, uses a float squared-distance
store before sqrt and ceil(distance*.4f) samples, with 2.5-unit steps and a final
exact current-task sample. X normalization retains its extended product until
step storage; Y/Z normalize to float before multiplying by2.5. Zero displacement
uses source.01f. A distance2.5 produces TWO samples with source.4f; replacing it
with a double.4 literal changes the boundary. Source float stores are explicit.
The service input must be the source task foot history, not display center or
an arbitrary previous rendered-frame position.

Each sample tests strict vertical branches in source order: top using the player
foot radius, underside using primary radius, then side using primary radius.
4199D0 expands X/Z dimensions and constructs all four yaw-rotated corners, then
4550A0 checks signed edge planes. This preserves an expanded rectangle, including
its corners and strict edges; no closest-point rounded box or sphere is invented.
The narrow four-unit vertical equality gap is retained. Hit kind/sample are
reported without fabricated displacement.

The CCL bridge applies existing418E70/418DE0 contact/cache/damage effects and
source top/side/underside flags. Actual category0/4 rules inhibit displacement.
An enabled push profile is rejected rather than silently ignoring an unported
response. The caller still owns broad-range/category eligibility, disabled
shape checks, task ordering and the original history producer. Dynamic land,
source pushing outside this checkpoint profile and native integration remain open.

Required cumulative host gate PASS. New focused cases include stationary hit/
miss, false sphere activation, expanded corners, exact/ULP side bounds, foot-radius
top, underside/equality gap, yaw, a24-unit sweep, .4f sample count, original contact
cache/status and inhibited-push rejection. Exact-owner static witnesses now
cover26 spans and assert category4/form3, shape dispatch and source constants.

Required exact-owner pose/import gate PASS. After the semantic version bump,
focused reader/version1 rejection and the raw-owner asset oracle passed again:
7 placements/42 poses/2016matrix components,max4.72656257e-6; texture59/60 and
both decoded rule records match raw owner values. Existing unrelated Sonic
matrices were not rerun after that header-only bump. Final native staged build
PASS, still byte-identical to WCA45 (440076B/1d8b24d8) because new components are
not referenced yet. No redundant emulator boot or native activation is claimed.

Corrected private version2 owner package150448B SHA256
6714ef87f89973c9d46c6efd4dfc3e92446d0f49e4980f353de9286b6251a015:
/private/tmp/sa3ds-wca48-checkpoint.sa3dspak. Not installed yet. Private targeted
source disassembly is /private/tmp/sa3ds-wca48-player-box.asm and
/private/tmp/sa3ds-wca48-box-polygon.asm. No owner data/code/build enters Git.

Next connect the actual task/art, source body contact/history, saved SET flags,
clock and restart payload to native play and run one activation/death/restore
fixture. All natural-course, dynamic-child collision, camera/physics/HUD/audio,
retail parity and physical Old3DS acceptance gates remain open.
