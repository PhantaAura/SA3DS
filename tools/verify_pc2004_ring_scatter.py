#!/usr/bin/env python3
"""Static owner witnesses for the scattered-ring task; no private code output."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x44fd10,1447):'0055a33013c206e752871fef8dc088027f40e9a5c1060388ab8bb351cf6fd082',
 (0x44f7a0,392):'f058831ab0add25bd3b04ab9739db86742a4b5ff1e1c5ba2a07b205644af287f',
 (0x44f960,294):'479265ab4679455373afff2b9482427177758de2e0010a5f39b4029ed4fc5237',
 (0x44fc80,144):'75bfbc9698461e50789e5a369eae99f5e6c556857790376093b7ced87b0aba22',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 assert struct.unpack('<5I',r(0x4502b8,20))==(0x44fd43,0x44fe00,0x450188,0x4502a7,0x45029b)
 #Compare memory constants as f32 values, including the source1/3 approximation.
 for a,v in [(0x7dce5c,.8),(0x7e05d8,1/3),(0x7e05d0,.08),(0x7e05cc,2.4),
             (0x7e05c8,13),(0x7e00dc,.04),(0x7e05d4,.28),(0x7dd540,.3),
             (0x7dd58c,.2),(0x7e05bc,3.44),(0x7e05dc,6.88)]:
  assert r(a,4)==struct.pack('<f',v),hex(a)
 assert r(0x45009b,3)==bytes.fromhex('884e01') #signed grace byte
 assert r(0x450283,5)==bytes.fromhex('66837e0664') #uint16 counter100
 assert r(0x450267,2)==bytes.fromhex('a802') #global frame bit1
 print('Scatter owner witnesses: PASS exact SHA,4 code spans,5-way dispatch,source constants/grace/blink/counter; static evidence only')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_ring_scatter.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
