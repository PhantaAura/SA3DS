#!/usr/bin/env python3
"""Owner-data-free integrity checks for the checked-in PC2004 metadata report."""
from pathlib import Path
import json


report = json.loads(Path("docs/PC2004_SONIC_ANIMDATA_REMAP.json").read_text())
assert report["schema"] == "sa3ds-pc2004-sonic-animdata-v2"
assert report["source_sha256"] == "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
assert report["initializer_va"] == "0x007CEC90"
assert report["runtime_animdata_va"] == "0x03C56210"
rows = report["animdata_rows"]
assert len(rows) == 146
assert [row["dense_id"] for row in rows] == list(range(146))
assert [row["source_action_slot"] for row in rows] == report["dense_to_source_action_slot"]

expected = {
    "Stand": (0, 1, 73, 3, 0, 0.0625, 0.5),
    "Walk1": (9, 3, 73, 10, 9, 0.25, 3.4000000953674316),
    "Run2": (13, 18, 73, 9, 13, 0.5, 0.5),
    "Fall": (18, 10, 73, 3, 18, 0.25, 1.0),
    "Land": (19, 11, 73, 4, 0, 1.0, 1.0),
    "SpinBall": (32, 21, 1, 3, 32, 0.5, 1.0),
}
for name, values in expected.items():
    row = report["core_movement"][name]
    actual = (row["dense_id"], row["source_action_slot"], row["instance"],
              row["property"], row["next_animation"], row["transition_speed"],
              row["animation_speed"])
    assert actual == values, (name, actual, values)

assert report["null_dense_animation_ids"] == [75, 76]
for dense, slot, prop, nxt, rate in ((1,122,4,2,0.25),(2,123,3,2,0.25),(29,22,4,1,1.0)):
    row=rows[dense]
    assert (row['source_action_slot'],row['instance'],row['property'],
            row['next_animation'],row['animation_speed']) == (slot,73,prop,nxt,rate)
print("PC2004 SonicAnimData v2 metadata report: PASS (146 exact rows)")
