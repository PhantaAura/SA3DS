# WCA52 — original game-over world fade

Previously zero remaining lives paused immediately in state8. The original
416610 branch instead calls42BF90 and enters8 with a separate42C350 fade.
At interval1 it increments alpha by6 BEFORE each draw, ignoring the ordinary
fade's delay counter. The43rd call sees entry alpha252 (>248), draws255 and
returns low16 -1. The source runs one final world update before clearing world
and rings and advancing to18. It does not subtract another life or reload.
The native stage flow now preserves those branches and defers teardown until
after that final world update. The native pause for the unimplemented screen
occurs in18, after the fade. This is not a completed game-over screen.

Focused host checks cover all43 alpha values, enabled world/disabled controls,
no early teardown, completion flags, state18 and unchanged signed lives. Existing
ordinary46/6/49 fade boundaries and death/protection checks remain in that runner.
Exact owner verifier now checks16 code spans including42BF90 and42C350 plus
state8's actual dispatcher target. No owner animation/import behavior changed.

## Next required screen work

Original18 initializes19 with selection0 and timeout1200. Original19 handles
confirm, timeout and input mask300C0 selection toggling; it renders42C350 and
runs13F50. Retry restores source lives4 and remembered stage/act before returning
through21; quit/timeout returns through21 with exit indication. Those branches,
original input handling, menu art/audio and the surrounding stage-entry loop
remain unimplemented. They must not be replaced by a guessed immediate retry.

The owner has GAMEOVER.PVM and GAMEOVER_E.PVM. The current strict importer reads
the former's seven images but rejects the latter at texture2 for directory/PVRT
mismatch. Original loader authority for that mismatch must be inspected before
changing the parser; silently substituting the other language is not justified.
No original assets or private disassembly are committed.

## Native verification

Required cumulative host gate PASS; exact owner16-span witness gate PASS; native
build PASS. Final artifact457836 bytes, SHA256
4accef903822139953ae2568750f9a748f3425bfe26f166469526e3768e76ed1.
That exact artifact passed Azahar2126.0 Old3DS/Vulkan: actual Saru row28 contact
with fixture inventory0/lives0, original122-update death task,43 game-over world
updates, state18/alpha255, zero restarts. Raster inspected: top screen opaque
black; diagnostic HUD lives0/flow18. This does not verify a retry/quit screen or
natural full-course game-over progression. An earlier row26 fixture failed to
produce damaging contact and timed out; the unchanged artifact was then restarted
with the previously recorded row28 death fixture. No gameplay values were tuned.

The English archive mismatch is specifically directory pixel format0 versus
PVRT pixel format2 at textures2 through6; sizes/data format agree. The directory
and payload must be checked against the original loader before relaxing strict
validation. NativeCPU texture upload/owner SD loading/boot diagnostics remain
unchanged. Physical Old3DS acceptance and movement-freeze confirmation remain open.
