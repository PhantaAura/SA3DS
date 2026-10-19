#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_pvm import SonicPVM


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_pc2004_sonic_pvm.py /path/to/SONIC.PVM")
    p=SonicPVM(Path(sys.argv[1]))
    assert p.texture_count == 16
    assert [e.index for e in p.entries] == list(range(16))
    assert p.entries[0].name == 'mtx_kuchinaka0'
    assert p.entries[0].global_index == 1000
    assert p.entries[-1].global_index == 1015
    assert {(e.pixel_format,e.data_format) for e in p.entries} == {(0,9),(1,9),(2,9)}
    assert {(e.width,e.height) for e in p.entries} == {(8,8),(16,16),(32,32),(64,64),(128,128)}
    # Decode every base mip and require exact RGBA buffer dimensions.
    for e in p.entries:
        rgba=p.decode_base_rgba(e.index)
        assert len(rgba) == e.width*e.height*4
    print('PC2004 exact SONIC.PVM parse/decode tests: PASS')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
