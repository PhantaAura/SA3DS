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
    (0x43F2A0,0x747):'ff5608c26ab1b4b0a0f45a8431a867edb20a416ea3eb3282f8a1840f9b6f9b6e',
    (0x4383A0,0xC):'e57e93e20110e2620aa31b341a614a607ed41e77a39eab761fe6ab5b79427c09',
    (0x4383D0,0x3B):'ad9404bc968cfada3ac0fb45004cb8db3c717146c3d7b18a2bb987123e6fe77b',
    (0x404DC0,0x87):'f19b70e44e2fea7ee14e5bf35ee76176f3cdd58fefee7fba364c83cb16c8acf3',
    (0x404FD0,0x48):'d0c337df0aa1e0a2b2b9ac4a0e2f6696d4eba8b7ed900feff93b6f8cd51e1cff',
    (0x405020,0x48):'cf50626a31a21cfb48bac61ba5896ca50de9ce2526f822cbea4fde6e172bdb41',
    (0x4050C0,0x104):'ab97780d15a52f1e2c0c1fe11079d6c4748e3db6c1da0a8e8c70a282060300e9',
}
def verify(path):
    read=verified_reader(path)
    for (address,size),digest in FUNCTIONS.items():
        if hashlib.sha256(read(address,size)).hexdigest()!=digest:
            raise ValueError('rotation evaluator/truncation witness mismatch')
    for address in (0x404C86,0x404C99,0x404CAD,0x43F627,0x43F648,0x43F66C):
        raw=read(address,5)
        if raw[0]!=0xE8 or address+5+int.from_bytes(raw[1:],'little',signed=True)!=0x644340:
            raise ValueError('rotation integer conversion call mismatch')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_pose_angles.py OWNER_SONIC_EXE')
    verify(sys.argv[1])
    print('PC2004 rotation quantization: PASS (ordinary weighted evaluator, separate transition capture and signed seam helpers; static, not retail trace)')
