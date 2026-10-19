# Checkpoint 0.0.9.0D — Real Sonic White-Space Render Gate

Status: **PASS (host render gate)**

### Core results
- Frozen 0.0.8.8.9 movement baseline remains intact.
- 19/19 Movement Lab scenarios pass in inherited regression.
- PC2004 real Sonic decode remains 73 normal nodes / 1,993 triangles / 149 ACTION slots / 146 populated motions.
- SA3C pose/draw submission: 73 visited nodes, 28 visible model nodes, 42 mesh draws, 1,993 triangles.
- SA3T: 16 textures / 232,192 RGBA8 pixel bytes in generated staging data.
- Host raster smoke: Action 0 frame 0 and walking source slot frame 0/frame 5 render successfully; frame 0 vs frame 5 differ.
- 146/146 populated motions have linear interpolation-function bits for this source profile.
- Dense 146-entry Sonic runtime animation remap reconstructed from exact 2004 `sonic.exe`; dense IDs 75 and 76 are null.

### Fixes made during this gate
- Corrected Basic quad triangulation to SA Tools-compatible `0,1,2` + `2,1,3`.
- Locked Basic UV normalization to `/255` from SA Tools evidence.
- Preserved Ninja object eval flags and XYZ/ZYX rotation behavior in pose traversal.
- Test runners disable Python bytecode writes so frozen baseline bytecode cannot be mutated by regression runs.

### Not claimed yet
- Citro3D/Old 3DS hardware rendering.
- Final retail lighting parity.
- Full material-state parity (all blend/filter/cull edge cases).
- Full physics/collision parity.
- Final gameplay-state -> dense runtime animation dispatcher binding.
