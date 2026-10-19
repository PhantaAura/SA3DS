# Next handoff after 0.0.9.0B

Current checkpoint: **0.0.9.0B — Physics-Synced Animation Gate**.

Do not modify frozen 0.0.8.8.9 physics/contact files.

Next exact byte-backed goal:

1. obtain exact `CHRMODELS_orig.dll` (7,647,232 bytes, SHA-256 `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`);
2. resolve `___SONIC_ACTIONS` and enumerate all 149 ACTION entries using proven +0x00 object / +0x04 motion fields;
3. enumerate unique motion pointers;
4. prove MOTION header/frame/key structure;
5. reverse executable dataflow far enough to map real gameplay animation states to ACTION indices;
6. connect those real actions to `sonic_animation_sync` without allowing animation to write physics;
7. target first visible real-source motion: Sonic idle if the evidence identifies it cleanly;
8. after normal Sonic works, first curated visual compatibility experiment should be Dreamcast Characters through the same neutral character/render path.
