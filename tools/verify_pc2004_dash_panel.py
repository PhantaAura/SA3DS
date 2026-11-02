#!/usr/bin/env python3
"""Exact-owner dash panel instruction/data witnesses; no live retail trace."""
from pathlib import Path
import hashlib,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
    (0x7a4450,0x269): 'b779be90bc9a4f03a1ddbd206a112f6115e5784a88c6a915dd875b253022b213',
    (0x441490,0xa1): '3c2b05ed5bbef1bf2b6d73ade358a522190931dc6555d9df65980c271dd6a6ec',
    (0x41b110,0x5a0): '686faa3f2b2be40e6a31c9f34fa4b27b1b97c3ea267fcf525169f3fcb5c50a33',
    (0x419470,0x130): 'fe88c8cea58e7081481e5e6f0be6653198511720a6e36ec933ec6f7c57186715',
    (0x644b20,0x180): '76c63ed5252082c506ad4e6f8743ac3795702d74fecede5266bc25f3147ca82a',
}
def verify(path):
    read=verified_reader(path)
    for (address,size),digest in RANGES.items():
        if hashlib.sha256(read(address,size)).hexdigest()!=digest:raise ValueError(hex(address))
    # Debug-property branch is unreachable in the exact release profile.
    assert read(0x4f88a0,3)==bytes.fromhex('33c0c3')
    assert read(0x7a454d,11)==bytes.fromhex('83c40485c07e0448894610')
    assert read(0x7a459d,4)==bytes.fromhex('f6420401')  # ground bit0, not bits0|1
    assert read(0x7a45ab,10)==bytes.fromhex('8b461085c07409385e01')
    assert read(0x7a4695,7)==bytes.fromhex('c7462c00006041')
    assert read(0x7a46ac,7)==bytes.fromhex('c7463000007042')
    for call,target in [(0x7a45f0,0x644b20),(0x7a45f5,0x644340),(0x7a460d,0x441490),
                        (0x44150b,0x43ec90),(0x7a4649,0x41c280)]:
        op=read(call,5);assert op[0]==0xe8 and call+5+int.from_bytes(op[1:],'little',signed=True)==target
    c=read(0x38cd410,48)
    assert c[:8]==bytes.fromhex('000670e000000000')
    assert struct.unpack('<3f',c[8:20])==(0,0,0)
    assert struct.unpack('<2f',c[20:28])==(10.,struct.unpack('<f',struct.pack('<f',4.44))[0])
    assert struct.unpack('<3i',c[36:48])==(0x4000,0,0)
    root,motion=struct.unpack('<II',read(0x8b8bc4,8))
    assert (root,motion)==(0x8b8b34,0x8b8bb8)
    assert struct.unpack('<IIHH',read(motion,12))==(0x8b8b88,8,3,2)
    print('PC2004 dash panel evidence: PASS exact task/direct-speed/CCL/ceil witnesses; no native or retail trace claim')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_dash_panel.py OWNER_SONIC_EXE')
    verify(Path(sys.argv[1]))
