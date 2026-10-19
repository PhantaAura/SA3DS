# Physics ↔ Animation Sync Contract — 0.0.9.0B

## Hard rule

**Physics owns gameplay. Animation only observes it.**

Per simulation tick:

1. input enters the frozen SA1 physics runtime;
2. physics updates `SA1PlayerState` at 60 Hz;
3. the animation observer reads the completed state;
4. that observation selects a presentation intent;
5. a future source-action resolver maps the intent to the real Sonic ACTION/MOTION proven from PC data;
6. the renderer samples/draws that motion;
7. animation never writes back into movement/collision state.

This prevents visual timing from changing Sonic's acceleration, jump, Roll, Spin Dash, slope contact, loop support, or any other frozen movement result.

## Physics-backed intent classes

These are **presentation intents**, not claimed SADX action IDs:

- `grounded_idle`
- `grounded_below_jog`
- `grounded_jog_to_run`
- `grounded_run_plus`
- `airborne_upright`
- `airborne_ball`
- `spindash_charge`
- `roll`

The grounded speed classes use the already-proven Sonic parameter evidence:

- `jog_speed = 0.46000000834465027`
- `run_speed = 1.3899999856948853`

These values are used only as **physics-backed classification boundaries**. 0.0.9.0B does **not** claim that SADX changes animation clips at these exact thresholds. That mapping remains UNKNOWN until the animation dispatcher / real ACTION table proves it.

## Same-tick invariants

- physics enters `SA1_ACTION_SPINDASH_CHARGE` → presentation intent is `spindash_charge` that completed physics tick;
- physics releases Spin Dash into `SA1_ACTION_ROLL` → presentation intent becomes `roll` that same tick;
- physics enters airborne jump with `ball_form=true` → presentation intent becomes `airborne_ball` that same tick;
- walking off an edge without ball form → `airborne_upright`;
- loop/wall orientation does not decide the Roll animation; `SA1_ACTION_ROLL` does.

## Timing

`SA3DSSonicAnimationObservation` carries:

- exact physics `sim_frame`;
- intent entry physics frame;
- ticks spent in the current intent.

This gives the eventual source-motion sampler a deterministic 60 Hz gameplay clock and prevents render-framerate drift.

The exact PC2004 `SonicAnimData` rows and the retail `PlayAnimation` dispatcher
at `0x0044A800` are now recovered. Properties 3/4/5/8 advance by the row's
`AnimationSpeed`; properties 6/7 subtract it. The native selector therefore
uses the exact fixed step for those properties. `InitCharacterVars` at
`0x00442750` binds property 9 to signed local `CharObj2.Speed.x` and properties
10/11 to stateful `CharObj2.SpindashSpeed`. Sonic's ground selector at
`0x00495CD0` refreshes that scalar as
`CharObj2.Speed.x + 20 * EntityData2[0x38]`; the jump transition at `0x00495E60`
instead assigns `5.0`. Property 9 multiplies its input by row speed; property 10
multiplies the scalar by row speed; property 11 uses the scalar directly. SA3DS
accumulates these steps once per completed physics tick. The verified
flat/upright path supplies the cleared zero residual; unrecovered non-flat
residual paths retain a flagged `0.5` fallback. Retail transition blending
remains unresolved and is not guessed.

## Air phase

Rising/apex/falling is exposed as an observation from world vertical velocity. It is diagnostic/presentation metadata only. It does not imply that SADX has three separate Sonic jump clips.
