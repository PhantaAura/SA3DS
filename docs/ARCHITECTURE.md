# Architecture summary

The 60 Hz simulation lives in `src/sa1/physics` and includes only conservative C and standard headers. `SA1Input` is logical input; the 3DS Circle Pad/deadzone adapter lives under `src/platform/3ds`. Physics queries an `SA1PhysicsWorld` surface callback rather than renderer state. The original Movement Lab geometry is a `src/game/stage.c` implementation of that callback, shared by host tests and the runtime.

State flows in one direction:

`device adapter or scripted input -> SA1Input -> fixed physics tick -> logical player state -> renderer/presentation`

Trace recording observes the logical input and post-tick state. Its host runner provides named repeatable scenarios; the comparator is ready for future user-captured reference traces but contains none.

`.sa3dspak` separates source conversion from runtime loading:

`validated source profile -> evidence-backed adapter -> neutral stage -> package -> allocation-free runtime reader`

The current built-in runtime package and host fixtures are synthetic/original. The runtime tracks package allocation and available linear memory behind the developer overlay. Other memory collectors remain explicit unknowns rather than guessed values.

Player logic and presentation identity are distinct types. A future presentation profile can resolve a different model without branching physics on an asset identity; no hidden profile content is implemented now.

## 0.0.8.7 arbitrary-contact layer

`SA1PhysicsWorld` now exposes an optional `query_contact(position, probe,
range)` callback. A contact contains a point, normal, signed distance, surface
id and flags. Worlds that only implement the older `query_surface(x,z)` API are
adapted automatically, so the new collision architecture does not invalidate
older host tests.

The player stores an explicit tangent frame (`ground_normal`,
`surface_forward`, `surface_right`). Non-flat grounded velocity is decomposed
into that frame, stepped there, then transformed back to world space. This is
the foundation required for arbitrary LandTable collision; renderer geometry
never defines physics state.


## 0.0.8.8 evidence isolation

The general-contact architecture remains independent of evidence provenance.
Recovered SADX steep-surface constants/branch decisions live in
`steam_surface_evidence.c` rather than being buried inside stage geometry or
contact code. This lets host tests freeze each proven/candidate behavior while
`SA1PhysicsWorld` remains usable by the white-space lab and future imported
LandTable collision.

The supported Steam EXE verifier is a development tool only; runtime code never
reads or depends on the Windows executable. Source conversion/research and the
3DS runtime stay separated.
