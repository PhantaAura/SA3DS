# Known limitations and unresolved evidence — 0.0.8.8.9

The pre-character **core movement math is frozen**, but SA3DS does not claim that
the entire game physics/collision stack is globally 1:1 yet.

Still gated:

- complete neutral reimplementation of the retail polygon candidate iteration
  and all contact-response vectors
- moving/dynamic collision objects and the pending-collision impulse producer
- remaining action/status-specific `PGetSpeed` branches
- airborne local frame after unusual wall/ceiling detach cases
- scripted paths, panels, tubes, stairs and other special movement handlers
- water/underwater movement
- retail camera behavior and camera trigger interaction
- imported Emerald Coast collision geometry and real-stage edge-case comparison
- Old 3DS Circle Pad deadzone, latency and frame-pacing calibration

The synthetic Movement Lab uses original geometry and a 1-unit fixture
stand-off. Retail Sonic's reconstructed csts radius is 5.0; those values are
kept separate intentionally until the real-stage collision layer exists.
