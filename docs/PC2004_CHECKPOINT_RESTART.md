# WCA46 — original checkpoint restart record

This checkpoint implements the portable session-record contract. It does not
activate native checkpoint posts or replace WCA45's runnable preview. Full
checkpoint gameplay/respawn acceptance remains open.

Authority is the owner's exact PC2004 EXE (SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74)
and private sonic.exe.c. Public X-Hax names were used only as address leads:
https://github.com/X-Hax/sadx-mod-loader/blob/master/SADXModLoader/include/SADXFunctions.h
Actual contracts were checked against targeted owner disassembly. No decompiled
code, owner data or original executable bytes enter Git.

44ED50 clears the nine-dword restart payload. 44ED70 compares the full
sign-extended level/act expression against the zero-extended stored16-bit key,
clears on mismatch, and returns the original32-bit validity word. 44EE70 saves
all three raw time bytes, actual player task XYZ and all three int32 rotation
words, then writes valid1 and the stage/act word. It does not capture the SET
post's position, infer scale from its parameters, clamp time, or store only yaw.
The portable representation is session state, not the original binary layout.

44EDB0 copies those task position/rotation values back, restores time through
4265F0, removes rings via425CC0/425BE0 and invokes441FD0 for player0. Its unrelated
level3/act2/Gamma override is explicitly outside this ordinary EC payload helper.
The native caller must perform those surrounding operations; it is not wired
by this source-only checkpoint. 414810 checks the record before choosing the
act start and later stores the initial/restarted player state in the ordinary
Sonic action stage. Thus source act-start fallback also seeds a restart record.

CheckPoint_Main44F540 is Emerald Coast registry19: flags15/list6/load-distance
squared360000. It checks ClipSetObject46C360 before dispatching states0..4.
Initialization creates two dynamic-land children, registers source CCI916A08,
sets initial post angular offsets4000/-4000 and checks persistent SET status.
The ordinary action1 path displays, updates child rotations and checks previous
CCL flag1 plus cached player0 identity. Activation44F430 sets action2, derives
signed post spin from the player's local speed, sets a300-count time display,
saves the ACTUAL player's task record, marks SET persistence and requests sound10.
The spin/children/persistence/collider/display executors are not implemented here.
Their task and clock order must be retained when integrating native posts.

Owner geometry research for the next integration remains private: root8C908C,
three nodes/models; post materials use texture59 before activation and60 after.
Both Sonic acts have seven total checkpoint placements (four/three) in the
original SET banks. Keep all converted CCI/model/texture/placement data external.

Verification: focused portable test PASS for exact task payload including signed
zero/all rotation words, raw clock bytes, repeat restore, act/level invalidation
and signed key boundaries. Exact-owner static verifier PASS: nine code spans,
clock/ring-reset calls, registry19 and five dispatcher destinations. Required
cumulative owner-data-free host gate runs with the new test. No pose/import or
native runtime change; no redundant pose/native build/emulator test is required.
No claim of native checkpoint activation, retail parity or hardware acceptance.

Private research: /private/tmp/sa3ds-wca46-checkpoint.asm and
/private/tmp/sa3ds-wca46-restart-time.asm, generated with installed
`xcrun llvm-objdump --disassemble --x86-asm-syntax=intel` and bounded address
ranges. REA current_document is unsupported by the Ghidra provider; list_documents
stalled and was cancelled. Do not retry those unchanged or run cold whole-game
analysis. The source helper is not evidence of a completed native gameplay loop.
