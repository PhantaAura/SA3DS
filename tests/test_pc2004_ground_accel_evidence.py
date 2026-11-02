#!/usr/bin/env python3
"""Exact-profile static witnesses for the portable clamp/local-speed helpers."""
import hashlib
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader

RANGES = {
    (0x44c923, 0x4c2): '25f65bce6b9d58a308ad6e86bdc93cfcb8ea9f5a8cd877e1bd0104316ad379ce',
    (0x443f50, 0x1e0): '7d8bbe900b6b83d06e059604213e6f8cb2bd1819c167bd9edeedc86b61be4eec',
}
SIGNATURES = {
    'positive residual subtract/store': (0x44cc07, 'd8642444d95b38d9442444'),
    'negative residual literal subtract/store': (0x44ca9c, 'd8642444d95b38d9442414'),
    'normal output addend after side clamp': (0x44cdb9, 'd94424208b44241cd844242c'),
    'zero-side retained threshold': (0x44cc6c, 'd98504010000d84b3cd84c2420d9442424'),
    'side drag threshold f32 store': (0x44ccb9, 'd98500010000d84b3cd84c2420d95c2444'),
    'speed normal direct integration': (0x444031, 'd94730d8473cd95f3c'),
    'speed side sum f32 store': (0x44403a, 'd94740d94734d84740d95c240c'),
}
def verify(path):
    read = verified_reader(path)
    for (address,size),digest in RANGES.items():
        assert hashlib.sha256(read(address,size)).hexdigest() == digest, hex(address)
    for label,(address,hex_bytes) in SIGNATURES.items():
        expected = bytes.fromhex(hex_bytes)
        assert read(address,len(expected)) == expected, label
    for address,target in [(0x49b33e,0x44b9c0),(0x49b349,0x443f50),
                           (0x49ae42,0x443f50),(0x4977cc,0x43bf90)]:
        ins=read(address,5)
        assert ins[0]==0xe8 and address+5+int.from_bytes(ins[1:],"little",signed=True)==target,hex(address)
    # Action selector is indexed by action-1; movement dispatch uses action.
    assert struct.unpack('<2I',read(0x49bfbc+8*4,8))==(0x49b324,0x49af71)
    assert struct.unpack('<I',read(0x49bfbc+12*4,4))[0]==0x49b324
    for address,value in [(0x7e0138,.71),(0x7e014c,.051),(0x7e0148,-.051),
                          (0x7dcb34,10),(0x7e0124,.87),(0x7dd578,.7),
                          (0x7dce5c,.8),(0x7dcc98,.5)]:
        assert read(address,4) == struct.pack('<f',value), hex(address)
if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: test_pc2004_ground_accel_evidence.py OWNER_SONIC_EXE')
    verify(sys.argv[1])
    print('PC2004 ground clamp/local-speed owner witnesses: PASS (static source ranges, stores and constants)')
