#!/usr/bin/env python3
"""Exact owner menu/PVRT contracts; static witnesses, no decompiled source."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x77f2f0,224):'e420dffdb1c774f74424ebc9510c667dd43901718a954c4b44bdd40b5bd0d846',
 (0x77f3f0,80):'253f4e898ad76f109e3859693383b2e1039a07603bbd7b7a0ec30b44c4ff7ef9',
 (0x77f910,192):'cbd3622a0307c2139ef7bdec35939c9327b81d8b10eb5e386b9affe1ecfbef22',
 (0x77fd80,304):'7a5643762ab5b6186b972587127451786d5eab46a660188a1102eace8452b80d',
 (0x42bfd0,896):'57857735d71ccf1dda3dd8f6cf07b3ac2ecc111f8a8ccf610febf36e96868083',
 (0x416610,3220):'4c1ef5c28dfbe460d40cdc63ba4efc1dc11bc4fd916108ac1fcea47ab3987e60',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 assert struct.unpack('<I',r(0x909f9c,4))[0]==0xc
 for a,v in ((0x7df2fc,.0078125),(0x7dce6c,255.),(0x7df24c,83.),(0x7df2ec,152.),(0x7df2e8,19.),(0x7df2e4,236.)):
  assert struct.unpack('<f',r(a,4))[0]==v
 print('Game-over owner witnesses: PASS exact SHA,6 spans,menu/draw/PVRT loader/accept mask/color constants; static only')
if __name__=='__main__':verify(Path(sys.argv[1]))
