#!/usr/bin/env python3
"""Exact-owner witnesses for ordinary PC2004 linear rotation sampling.

Hash-based static evidence, not an original-executable runtime trace.
"""
from pathlib import Path
import hashlib
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
FUNCTIONS={
    (0x404C10,0xAF):'ca12fedc86c0979b83c53e8f077b21e18cd90df8d2488c5c6c1914b5e2bdc5db',
    (0x644340,0x75):'5333b186c02836974c6f792303aeb2c00d856316b93ccbbe65f51def6ae661b4',
}
def verify(path):
    read=verified_reader(path)
    for (address,size),digest in FUNCTIONS.items():
        if hashlib.sha256(read(address,size)).hexdigest()!=digest:
            raise ValueError('rotation evaluator/truncation witness mismatch')
    for address in (0x404C86,0x404C99,0x404CAD):
        raw=read(address,5)
        if raw[0]!=0xE8 or address+5+int.from_bytes(raw[1:],'little',signed=True)!=0x644340:
            raise ValueError('rotation integer conversion call mismatch')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_pose_angles.py OWNER_SONIC_EXE')
    verify(sys.argv[1])
    print('PC2004 rotation quantization: PASS (exact signed-key weighted evaluator and truncation helper; static, not retail trace)')
