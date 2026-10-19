#!/usr/bin/env python3
from pathlib import Path
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.importers.pc2004_chrmodels import SonicDecoder


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_pc2004_chrmodels_decode.py /path/to/CHRMODELS.DLL")
    dec = SonicDecoder(Path(sys.argv[1]))
    report = dec.report()

    assert report["tables"] == {
        "models_entries": 11,
        "models_nonnull": 10,
        "objects_entries": 79,
        "objects_nonnull": 77,
        "actions_entries": 149,
        "actions_nonnull": 146,
        "null_action_indices": [24, 25, 26],
    }
    assert report["hierarchies"]["normal_action0_nodes"] == 73
    assert report["hierarchies"]["distinct_action_roots"] == 6
    assert report["models"]["normal_hierarchy_distinct_models"] == 28
    assert report["models"]["normal_hierarchy_points"] == 1161
    assert report["models"]["normal_hierarchy_triangles"] == 1993
    assert report["motions"]["count"] == 146
    assert report["motions"]["type_distribution"] == {"3": 136, "7": 10}
    assert report["motions"]["interpolation_distribution"] == {"2": 136, "3": 10}
    assert report["motions"]["node_count_distribution"] == {"73": 126, "74": 1, "1": 19}

    # First action: concrete byte-backed witness already inspected in RE.
    action0 = dec.actions()[0]
    assert action0 is not None
    assert action0.root_object_va == 0x1056AF50
    assert action0.motion.frame_count == 8
    assert action0.motion.type_bits == 3
    assert action0.motion.interpolation == 2
    assert action0.motion.node_count == 73

    print(json.dumps({
        "status": "PASS",
        "source_sha256": report["source"]["sha256"],
        "normal_nodes": report["hierarchies"]["normal_action0_nodes"],
        "normal_points": report["models"]["normal_hierarchy_points"],
        "normal_triangles": report["models"]["normal_hierarchy_triangles"],
        "actions_nonnull": report["tables"]["actions_nonnull"],
        "motions": report["motions"]["count"],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
