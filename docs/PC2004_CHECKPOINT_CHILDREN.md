# WCA50 — checkpoint children are inert tasks

The prior checkpoint notes incorrectly called the two child tasks dynamic-land
objects and listed physical post collision as a missing executor. Inspection of
the exact owner's PC2004 executable contradicts that interpretation.

Both creation sites in parent44F540 pass callback51F680 to40B940 with flags2.
Callback51F680 consists of a single RET instruction. Constructor40B940 creates
a list7 child, links it to its parent, copies position and all three rotation
words when task data exist, and copies the parent's field30. It does not supply
a land model or register physical terrain. The checkpoint's44F070 applies two
rotated offsets to those inherited positions;44F430 updates child rotations.
The parent separately owns the source category4 CCI activation box.

The bounded parent initialization/update code and inert callback therefore do
not justify inventing post obstacles or a dynamic-land executor. This is static
owner-binary evidence, not observation of the PC game's running scheduler.
Runtime patches/mods and other object families are outside this claim. The
portable task can retain sync requests without allocating tasks whose callbacks
have no behavior. Native diagnostic naming now says child synchronization.
The legacy external package land_offsets field retains its binary layout.

The exact-owner verifier now checks34 spans, including the132-byte constructor
and one-byte inert callback, plus both actual parent callback arguments.
Private disassembly remains outside Git. CPU texture upload, SD owner loader,
boot diagnostics and gameplay equations are unchanged. Full contact-history,
effects/audio/time-glyph and natural route fidelity remain open.

Native rebuild passed and produced exactly the previously replayed WCA49 binary:
456300 bytes, SHA256
4a2a9de476c38ebf6d0865f78f2fda56a5c0d99ec553eac1b616eb834deba953.
No new emulator replay or unchanged pose/import matrix is needed for this
interpretation/diagnostic-only change.

Required cumulative owner-data-free host gate: PASS. The exact-owner34-span
verifier: PASS. No owner pose/import data or sampling behavior changed.
