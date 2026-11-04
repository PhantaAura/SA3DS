# WCA62 — whale horizontal movement and interaction tail

Portable `whale_common.[ch]` reconstructs502754..5029BD after the existing
constructor/action phases. The animated tip calculated by50264A..502754 is
still a caller input. This is source-rule progress, not a native whale chase.

The source reads player0/history offset0, rather than current task position.
Horizontal distance is square-rooted after a float sum store. Above400 units,
speed increases by float.1 only below12; between200 and400 it increases below6
and decreases otherwise; at/below200 it decreases above1 and sets1 otherwise.
These are comparisons and additions, not clamped interpolation: overshoot and
oscillation are retained. Shared kill exactly1 forces speed2 without reading
history. Yaw-relative sine drives lateral wobble, including the divide-free
zero-speed branch. Forward steps/scalar duplicate/shared movement retain the
recovered float-store boundaries; positionZ consumes the unrounded delta.
Host double is a bounded approximation to x87, not a proof of all80-bit words.

Collision center uses the tip sampled BEFORE horizontal movement; action1
uses(0,-100000,0). Previous CCL status bit8 invokes441AD0, which returns a
character ID from a recognized player object. ID0 requests player0 velocity
(headingX*8,2,headingZ*8); ID1 does not request a player1 bounce. Bounce precedes
collision enqueue. The unsigned16 clock then increments/wraps. Above18,
unless action2, two source RNG calls scatter a wave within250 units in X/Z;
wave arguments0,1,13 and reset-to0 are preserved. Constructor clock18 therefore
emits on the first tail. Display follows, including a diving deletion request;
only the phase far-distance early exit stops the whole tail.

Checks: required cumulative host PASS; native code-only build PASS. Focused
host cases cover boundary oscillation/overshoot, kill equality/history service,
pre-movement collision, recognized character distinction, callback order,
constructor sentinel/wave, dive RNG suppression, counter wrap and zero-speed
wobble. Exact owner12-span witness plus10 constant words PASS; generated trigger
package stays byte-identical720B/acb1f158. No pose/import format changed, so the
unchanged pose matrix was not repeated. No owner code/data is included here.

The helper is not referenced by native gameplay and is linker-discarded.
Native artifact remains470768B/013d0577. Tip pitch-track interpolation,
vertex/normal shape animation, player history/CCL services, actual wave/audio,
shared actor lifetime, pier, camera and stage executor wiring remain open.
Full CP6 and CP1–CP9 are not complete. Lighting raster failure is recorded in
PC2004_SONIC_LIGHTING_NATIVE.md; Desktop preview stays WCA56.
