# SA3DS 0.0.9.0G — White Zone Validation Gate

## Input

Reconstructed from the real code-only `SA3DS_0.0.9.0F_WhiteZoneParityFoundation.zip` recovered from the persistent SA3DS library. The transient overnight H1-H4 archives were unavailable, so this checkpoint does not claim byte-for-byte inheritance from those sandboxes.

## What 0G adds

- 10 deterministic White Zone sections with dedicated TURN and final AIR CONTROL / LANDING.
- 24-unit course width and 10-unit measurement markers retained.
- Selected-zone fallback wired into the actual physics world reset position.
- Portable parity HUD snapshot layer.
- FOLLOW and REFERENCE camera states.
- Extended White Zone trace format with test, movement/contact and animation fields.
- Strict retail comparator aligned by `(test_id, frame)`.
- 12 deterministic movement protocols.
- 1,024-state animation observer/selector non-mutation stress.
- Explicit Emerald Coast acceptance gate and behavior-by-behavior parity matrix.

## Evidence state

`PHYSICS_1TO1 = false`.

No `.3dsx`, physical Old 3DS observation, or new live SADX trace is claimed in this checkpoint. devkitARM and canonical owner payloads were not mounted in the execution environment used to rebuild the ZIP.

## Validation

See `TEST_RESULTS_0.0.9.0G.txt` for the exact observed pass set and environment limitations.
