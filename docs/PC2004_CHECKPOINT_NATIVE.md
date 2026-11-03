# WCA49 — native checkpoint activation and restart

The native runtime now loads the external version-2 checkpoint package, draws
original post components and runs checkpoint tasks before shared player collision.
Activation consumes the preceding CCL contact cache, preserves SET flags, and saves
player task-foot position, canonical rotations and all three course-clock bytes.
The ordinary death/fade/lives/reload path restores that record. The normal Sonic
clock ticks after world tasks with 60/60 rollover, a 99:59:59 cap and source-style
signed-byte comparisons; it remains separate from the monotonic source-frame count.

## Evidence

Required cumulative host gate and exact-owner pose/import gate passed. The owner
asset oracle checked seven placements, 42 callback poses and 2016 matrix components
(maximum error 4.72656257e-6). The extended restart verifier checks 32 owner code
spans. Native build used tools/build_3ds.sh. Final artifact: 456300 bytes, SHA256
4a2a9de476c38ebf6d0865f78f2fda56a5c0d99ec553eac1b616eb834deba953.

That exact artifact passed the bounded Azahar 2126.0 Old3DS/Vulkan fixture at
act-0 SET row 90: one save, 83 contacts, 1251 post draws, zero restore-position
error, exact saved clock bytes, SET persistence, lives 4 to 3, one restart, and
12 ticks of resumed control. It moved 8.079 units away before an injected zero-ring
damage report; ordinary death command/task and stage fades then ran. This is not
an enemy-caused death or a natural course traversal. The final raster was inspected.
Earlier candidates failed the displacement assertion at 2.016 and 4.333 units;
the final fixture backs away along the traversed approach and waits for measured
8-unit displacement. Gameplay speed was not changed to satisfy the fixture.

## Remaining fidelity work

Dynamic-land children are requested/counted, not executed as collision objects.
Sound, effects and original time glyph executors remain incomplete. The current
player bridge uses canonical contact-frame angles, not recovered original angle
history/Euler branch selection. Full original CCL history phase, scheduler order,
camera clipping and player initialization remain unproven. The retained baseline
player reset starts grounded; ordinary subsequent contact resolves support.
Full course traversal, other death causes/game over, physics, lighting, retail
visual parity and physical Old3DS acceptance remain open. CPU texture upload,
external owner-pair loading and boot diagnostics are preserved. Owner-generated
packages and native artifacts remain outside Git.
