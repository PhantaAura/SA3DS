# WCA51 — original respawn protection task

The ordinary restart helper44EDB0 calls441FD0 after restoring the player's
saved state and time and removing rings. That allocates a list0 task with
callback441F80. The callback increments its unsigned16 counter BEFORE testing
against120. Updates1 through120 set player-work flags6 bit8000. Update121 clears
that bit and destroys the task. Unrelated flags remain intact. Missing player
work does not stop the counter or destruction;16-bit overflow is retained.
This differs from the death task's old-counter/122-update boundary.

The native reload now creates the protection task when a valid restart record
was used. It runs before Sonic damage preparation whenever the world runs,
including fade-in updates while controls are disabled. It does not start a
fresh120-count window when controls return. World teardown/debug reset/cleanup
remove the private native task list. Allocation failure leaves protection
uncreated, consistent with the source's optional task allocation. Full allocator
and global task ordering equivalence are not claimed.

The existing damage bridge consumes flags6: protection suppresses damage and
zeros the hurt timer. Existing Sonic CCL damage-byte production receives the
same flags. This is separate from post-hit blinking and its hurt timer; no
new flashing animation is introduced.

## Verification

Focused protection checks cover every update through121, preservation of
unrelated flags, missing-player expiry, UINT16 wrapping, inactive tasks and the
real damage preamble while protected. Required cumulative host gate PASS.
Exact-owner static verifier PASS35 spans, including441F80. Native build PASS.
No pose/import behavior changed, so the unchanged owner pose matrix was not rerun.

Native artifact:457164 bytes, SHA256
6f8c03b77722588ac9c5c24c392bf15176890af4c33ba878229a4157f0e951e8.
Native checkpoint fixture now waits for protection expiry before its final gate,
requiring one creation/one completion and a cleared8000 flag as well as the
existing save/restore/time/SET/lives/death/control assertions.

Full-course/physical hardware/retail task-order and input trajectory acceptance
remain open. Sound, effects, checkpoint glyphs and other death causes/game over
remain incomplete. CPU texture upload, owner SD loading and boot diagnostics
are preserved; no owner data or native binary enters Git.

Exact-artifact Azahar2126.0 Old3DS/Vulkan fixture PASS: reload sourceFrame394;
protection counter1/flags8000 at395; fade controls return after443; counter121/
flags0000/destroy1 at515. Final gate:508 world ticks, one checkpoint save,136
contacts,1401 draws, zero restore-position error, exact saved clock, SET
persistence, lives3/one restart,65 ticks resumed control. Raster inspected with
original posts, Sonic and beach. This verifies the native lifetime/flag path;
no enemy hit was introduced during the protected interval. Damage suppression
was checked through the real portable damage preamble on the host. Initial
zero-ring damage report remains injected after8.079-unit departure, not a
natural enemy/death traversal. The first launch used the default profile and
was closed; this PASS used the isolated XDG profile and private SD assets.
