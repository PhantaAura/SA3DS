# WCA67 — original position-history service and native player-tail capture

Whale5022C0 reads4419C0(player0,offset0), not an arbitrary live task-position
fallback. The source ring cursor is a byte:43B960 increments it BEFORE writing,
and4419C0/441A40 subtract a byte offset with wrap. Cold unused slots are zero;
there is no age clamp or warm-up prefill.43B920 clears the player tables without
clearing the position history or cursor. Source also stores task rotation in a
parallel ring, but this checkpoint deliberately implements only the position
channel the whale needs. No synthetic angle history is presented as original.

`player_position_history.[ch]` provides one player's256-position ring. The caller
supplies player presence; absent reads leave output untouched and return0.
Optional null outputs/setter inputs preserve the source no-copy success case.
The nonadvancing setter addresses cursor-offset. Position words are copied
exactly, including signed zero and other bit patterns; no vector arithmetic or
normalization changes stored values. Rejecting a record request while absent
is a port API guard; the source writer assumes a valid player task at its call.

Both Sonic tail branches49BF7B/49BFAD call446F10 after Sonic's display/CCL work.
446F61 calls43B960. Native main now captures the completed ordinary port task
origin after movement/animation and before list2/later world object updates.
It uses a separate static3KiB ring, once per simulation tick rather than per eye
or rendered frame. No existing physics state is mutated. The history remains
separate from later spring/panel/object commands that can affect live state.
The source's exceptional scripted changes after this tail point and complete
player lifetime/angle scheduling are not verified by this ordinary integration.

## Evidence and candidate

Required cumulative host PASS. Focused513-write/all256-offset checks cover cursor
wrap, cold slots, retained bytes across absence, offset setter, no age clamp,
optional pointer behavior and exact float-word copies. Exact-owner verifier
checks six source spans plus Sonic display/tail/history call targets and cursor
operations. No importer/pose asset changed, so the unchanged pose matrix was not
rerun; its gate now includes this static witness for future owner runs.

Native build via tools/build_3ds.sh PASS. Code-only candidate471008B SHA256
 a0b63f49de98835e97242f821851fe3ef30caf7be1cd8791cc079db5aed6e7b7
has NOT run in Azahar: the current UI observation reports the Mac locked and
an unlock request is pending. This is a new artifact, not the prior013d0577
candidate. The private rejected light profile remains inactive. The inspected
Desktop preview stays WCA56/78f5506a; no visual promotion or hardware claim.

The native history records now, but whale services do not consume it yet.
Next: strict existing trigger-package reader and source presence/contact gates,
shared whale lifetime/native loader and event services, then draw/interaction
inspection and pier/camera. Full CP1–CP9 is active and incomplete. Old3DS
movement-freeze acceptance and retail trajectories remain unproven.
