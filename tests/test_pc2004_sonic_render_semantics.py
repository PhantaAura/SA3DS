#!/usr/bin/env python3
"""0.0.9.0D source-semantics gate for real Sonic rendering."""
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.importers.pc2004_chrmodels import SonicDecoder, MESHSET_SIZE
from tools.build_pc2004_sonic_sa3c import (
    parse_model_raw,
    source_uvs,
    strip_stream,
    triangulate_quad,
    triangulate_strip,
)


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_pc2004_sonic_render_semantics.py CHRMODELS.DLL")
    dec = SonicDecoder(Path(sys.argv[1]))
    actions = dec.actions()

    assert [i for i, a in enumerate(actions) if a is None] == [24, 25, 26]
    populated = [a for a in actions if a]
    assert len(populated) == 146

    pair_counts = {}
    function_bits = {}
    for action in populated:
        motion = action.motion
        # 0x000F is the pointer/count-pair count in Ninja's high-16 word.
        # 0x00C0 is the interpolation-function mask.
        pair_counts[motion.pair_count] = pair_counts.get(motion.pair_count, 0) + 1
        function_bits[motion.interpolation_function_bits] = function_bits.get(motion.interpolation_function_bits, 0) + 1
        assert motion.pair_count == (2 if motion.type_bits == 3 else 3)
        assert motion.interpolation_function_bits == 0  # NJD_MTYPE_LINER
    assert pair_counts == {2: 136, 3: 10}
    assert function_bits == {0: 146}

    # Pin the exact SA Tools triangle-list conventions used by the converter.
    assert triangulate_quad([10, 11, 12, 13]) == [10, 11, 12, 12, 11, 13]
    assert triangulate_strip([10, 11, 12, 13], False) == [10, 11, 12, 12, 11, 13]
    assert triangulate_strip([10, 11, 12, 13], True) == [11, 10, 12, 11, 12, 13]

    # Check the actual Sonic Basic-model corner UV domain.  SA Tools' little-
    # endian Basic UV reader divides these signed-16 values by exactly 255.0.
    models = set()
    for action in populated:
        for obj in dec.hierarchy(action.root_object_va):
            if obj.model_va:
                models.add(obj.model_va)
    uv_values = []
    quad_count = 0
    strip_count = 0
    for mv in models:
        _p, _n, _np, meshsets_va, _mat, nmesh, _nmat, *_rest = parse_model_raw(dec, mv)
        for mi in range(nmesh):
            raw = dec.pe.read_bytes(meshsets_va + mi * MESHSET_SIZE, MESHSET_SIZE)
            type_material, poly_count = struct.unpack_from("<HH", raw, 0)
            ptrs = struct.unpack_from("<IIIIII", raw, 4)
            primitive = type_material & 0xC000
            if primitive == 0xC000:
                strips = strip_stream(dec, ptrs[0], poly_count)
                corner_count = sum(len(s[1]) for s in strips)
                strip_count += poly_count
            elif primitive == 0x4000:
                corner_count = poly_count * 4
                quad_count += poly_count
            else:
                raise AssertionError(f"unexpected primitive {primitive:#x}")
            for u, v in source_uvs(dec, ptrs[4], corner_count):
                uv_values.extend((u, v))

    assert strip_count == 941
    assert quad_count == 22
    assert min(uv_values) == 0
    assert max(uv_values) == 255
    assert all(0 <= v <= 255 for v in uv_values)

    print(
        "PC2004 Sonic render semantics: PASS "
        f"motions={len(populated)} linear={function_bits[0]} "
        f"strips={strip_count} quads={quad_count} uv=[{min(uv_values)},{max(uv_values)}]"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
