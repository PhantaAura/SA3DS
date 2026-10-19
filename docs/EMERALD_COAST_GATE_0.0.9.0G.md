# Emerald Coast Source Decode Acceptance Gate

White Zone source-side validation is complete enough to move to the first real-stage decode once the measurement pipeline has been exercised on user hardware.

Required before declaring the gate exercised:

1. Generate owner-only `sonic.sa3c` / `sonic.sa3t` from the user's supported source data.
2. Build `runtime_0_0_9_0G` with devkitARM on the Mac.
3. Boot the resulting build on an Old 3DS / Old 3DS XL.
4. Verify selected-zone reset, TURN lane, FOLLOW/REFERENCE camera switching, developer HUD, and CSV recording.
5. Capture at least one corresponding live SADX protocol and run `tools/compare_white_zone_traces.py` against the SA3DS trace.
6. Confirm the comparator aligns by `(test_id, frame)` and either reports exact agreement or pinpoints a reproducible first divergence. A mismatch does not justify a 1:1 claim, but it does validate the measurement pipeline.

Once those checks work, Emerald Coast Source Decode may begin. Full global physics certification is **not** required before stage decoding; real-stage collision, slope, camera and object evidence is needed for the next parity layer.
