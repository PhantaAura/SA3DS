#!/usr/bin/env python3
from pathlib import Path
import tempfile
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.build_pc2004_sonic_sa3c import build_package
from tools import sa3c


def main():
    if len(sys.argv)!=3:
        raise SystemExit('usage: test_pc2004_sonic_sa3c_build.py CHRMODELS.DLL SONIC.PVM')
    package,report=build_package(Path(sys.argv[1]),Path(sys.argv[2]))
    parsed=sa3c.parse(package)
    assert parsed['source_sha256']==report['source_chrmodels_sha256']
    c=report['counts']
    assert c['NODE']==223
    assert c['MODL']==87
    assert c['ANIM']==149
    assert c['TEXS']==16
    assert c['VRTX'] < 65536
    assert c['INDX'] > 0 and c['CHAN'] > 0 and c['KEYS'] > 0
    assert parsed['sections'][b'NODE'].count==223
    assert parsed['sections'][b'ANIM'].count==149
    # Determinism: identical inputs produce byte-identical package.
    package2,report2=build_package(Path(sys.argv[1]),Path(sys.argv[2]))
    assert package2==package
    assert report2['package_sha256']==report['package_sha256']
    print('PC2004 Sonic -> pointer-free SA3C deterministic build: PASS')
    print('SA3C counts:',c)
    print('SA3C bytes:',report['package_bytes'])
    return 0

if __name__=='__main__':
    raise SystemExit(main())
