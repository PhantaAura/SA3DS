# SA3DS 0.0.9.0B — Physics-Synced Animation Gate

## Status

Code-only checkpoint. No Sega models, textures, animations, audio, or executable bytes are included.

## New in 0.0.9.0B

- read-only Sonic animation-intent observer driven from completed frozen physics ticks;
- exact Sonic `jog_speed` / `run_speed` evidence used only for physics-backed locomotion classes;
- same-tick Spin Dash charge -> Roll presentation transition;
- same-tick jump -> airborne-ball presentation transition;
- airborne upright/ball distinction from actual `ball_form`;
- loop/wall-safe Roll selection from actual action state;
- 60 Hz `sim_frame`-based presentation clock;
- strict tests proving the observer does not mutate `SA1PlayerState`;
- all-19-scenario animation-intent trace report;
- exact PC2004 `___SONIC_ACTIONS` direct constant-slot atlas.

## Evidence boundary

### CONFIRMED
- physics owns all gameplay state;
- animation observer is presentation-only;
- direct action-table slots 0,1,2,3,4,5,6,7,20,77,100,122,123 are referenced by the exact supported PC2004 executable in the scanner's narrow immediate-access pattern;
- those direct references are real code evidence but have no action-name semantics assigned yet.

### UNKNOWN
- real idle/walk/run/jump/roll/Spin-Dash ACTION indices;
- motion record structure and count;
- frame/key layout;
- source interpolation;
- source animation playback-rate logic;
- exact locomotion animation transition rules.

## Next gate

Mount and verify exact `CHRMODELS_orig.dll`, enumerate the 149 real Sonic ACTION entries, collect unique motion pointers, prove the MOTION/key layout, then connect real source actions to the already-tested physics-synced intent bridge.
