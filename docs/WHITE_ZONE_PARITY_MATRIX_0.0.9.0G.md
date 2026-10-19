# SA3DS 0.0.9.0G — White Zone Parity Matrix

Global `PHYSICS_1TO1 = false`.

| Behavior / evidence | 0G classification | Basis / remaining gap |
|---|---|---|
| White Zone geometry, 24-unit width, 10-unit markers, 10 contiguous zones | TEST FIXTURE VERIFIED | Original deterministic lab geometry; not retail stage geometry. |
| Selected-zone reset including real physics out-of-bounds path | TEST FIXTURE VERIFIED | Host test drives the actual physics reset branch and returns to TURN. |
| Flat acceleration | SOURCE-BACKED / PARTIAL | Deterministic candidate plus recovered SADX/Steam arithmetic; retail live trace still needed. |
| Coast / release | SOURCE-BACKED / PARTIAL | Deterministic candidate; retail live trace pending. |
| Braking / reverse | SOURCE-BACKED / PARTIAL | PC-2004 break/inertia evidence plus deterministic protocol; retail trace pending. |
| Low-speed turning | SOURCE-BACKED / PARTIAL | Dedicated TURN fixture and deterministic protocol; exact retail timing/rates pending live trace. |
| High-speed turning | SOURCE-BACKED / PARTIAL | Dedicated TURN fixture and deterministic protocol; exact retail timing/rates pending live trace. |
| Jump tap | SOURCE-BACKED / PARTIAL | Deterministic airborne/landing sequence; exact retail frame parity pending. |
| Jump hold | SOURCE-BACKED / PARTIAL | Deterministic held-jump sequence; exact retail frame parity pending. |
| Apex frame / landing frame | LIVE CAPTURE PENDING | 0G records exact candidate rows, but they are not retail-certified. |
| Spin Dash charge/release | SOURCE-BACKED / PARTIAL | Exact recovered behavior/constants where established plus deterministic state protocol. |
| Roll entry/exit | SOURCE-BACKED / PARTIAL | Deterministic state protocol; exact retail transition timing still pending. |
| Uphill / downhill surface response | SOURCE-BACKED / PARTIAL | General-surface and support-order evidence plus deterministic fixtures. |
| Air control | SOURCE-BACKED / PARTIAL | Recovered airborne candidate behavior; exact retail trace pending. |
| Dense Sonic runtime animation ID → ACTION slot mappings | SOURCE-BACKED / PARTIAL | Known recovered mappings are tested. |
| Animation observer cannot mutate physics | VERIFIED ARCHITECTURE INVARIANT | 1,024-state `memcmp` stress test plus runtime guard. |
| Walk/jog/run animation switch policy | NOT VERIFIED | Selector deliberately reports `PROVISIONAL`; `jog_speed` and `run_speed` are not claimed as retail animation thresholds. |
| Live SADX frame-by-frame equality | LIVE CAPTURE PENDING | Comparator is ready; no new live retail capture was available in this run. |
| Old 3DS performance / memory / controls | HARDWARE PENDING | Runtime HUD reports draw calls, triangles and free linear memory, but no physical 3DS observation is claimed. |
| 0G `.3dsx` | BUILD PENDING | devkitARM is not available in this execution environment. |
