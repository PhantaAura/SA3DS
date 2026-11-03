#!/usr/bin/env python3
"""Bind shared enemy damage/collision-query rules to the owner's exact executable."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={(5038128, 193): '83beabaee8762c6a9835a09aef028aab29d61b6f17768542003108dc6d1f562f', (5038048, 80): '6ad14318f549d3f43a5519d943886025b13aa646cb86e72f3a0ddff4aef5b87b', (4982096, 112): 'bc54355dff72df860dc5162d3d268f8fc9fe0b7c60d74a435c926b460eebfe93', (4307440, 89): '29d45ea2c2bc170c9590c3a87fa57c138560b8cd8b66723ee94b8cf49e571965', (4310752, 97): '64fefe2ec3fcd2f4cb81d890e0fe890bc55579bfb360af5884385078807249ff', (4310864, 98): '63be9bddf50c598c26a0500942f8ce3fa6d5177906136b141bbdee2a44570ec7', (7896256, 38): '8394622b2f31ae87570f34c5cd791dfa7338a9a82f7cfec0533069c7bd7a74e0'}
def verify(exe):
    read=verified_reader(exe)
    for (address,size),expected in RANGES.items():
        assert hashlib.sha256(read(address,size)).hexdigest()==expected,(hex(address),size)
    assert struct.unpack('<f',read(0x4ce055,4))[0]==struct.unpack('<f',bytes.fromhex('3333b33e'))[0]
    assert struct.unpack('<f',read(0x4ce0a3,4))[0]==struct.unpack('<f',bytes.fromhex('3333333f'))[0]
    assert read(0x4ce076,2)==bytes.fromhex('6a0a')
    assert read(0x4ce097,3)==bytes.fromhex('f6c510')
    print('Original enemy damage: PASS exact-owner7 helper bodies and squash/recoil/score/strong-attribute operands; static identity, not retail execution')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_enemy_damage.py OWNER_SONIC_EXE')
    verify(Path(sys.argv[1]))
