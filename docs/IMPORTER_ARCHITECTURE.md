# Importer architecture and evidence gates

Source adapters validate one exact source profile, parse only evidence-backed structures, and emit a source-neutral `NeutralStage`. The package writer then maps neutral chunks to `.sa3dspak`; the runtime sees no executable address or source-build distinction.

The clean Steam and 2004 PC builds are deliberately separate profiles. The current Steam inventory proves a file fingerprint only. It does not provide executable bytes, structure contents, or addresses for `___LANDTABLEEC`, `___SONIC_OBJECTS`, or `___SONIC_MOTIONS`; all remain **UNKNOWN**.

The future Emerald Coast path is:

`validated Steam bytes -> proven marker/root -> validated LandTable traversal -> neutral stage -> .sa3dspak`

Existing PVM/PVRT, SET, and CAM work can become source-adapter components after its code/evidence is supplied. No parser was fabricated in this checkpoint. A selected mod gets its own adapter layered on a validated base profile; arbitrary Windows mod DLLs are never executed by the 3DS runtime.
