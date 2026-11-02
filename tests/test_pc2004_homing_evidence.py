#!/usr/bin/env python3
"""Exact owner homing collection/pursuit and source math witnesses."""
from pathlib import Path
import hashlib,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x418be0,0x120):'f4e6f70b8d2c7da332c74420d90a8791bce902cfeebf08baf1e8759f255d2919',
 (0x418d00,0x76):'a3b9f5c651534c4f087b841961a934b9d098e9c7fc51ada3ffec99b1e862d414',
 (0x418b30,0x2f):'a4de8016354cc6b188722327caf0e73ca535ca513c62ae6e734f9220a80a53ed',
 (0x438350,0x50):'9be6c87e34045b93d699848eb77cf0bdaf28fc2865c90b0734d0dbb04b925ffd',
 (0x4383b0,0x13):'06ee468f7aed3a01c0a6c7e51e3b0e129570fbb3def05f438293a7c6bc552f5c',
 (0x6443bf,0x23):'188428aa92fc9dcb10257abe33b7876e4526b1a0327ad0b04220aa2b4d88719c',
 (0x4b83f0,0x76):'d62b3950d9a761f1f4b013b8eea1ec0466b857fe8026955b462aeb90c40c8af5',
 (0x492300,0x410):'f0dfac60151d1eebc1fe5834ec1fd8402e53b187b289164b7a8dc533c8582c71',
}
def verify(path):
 r=verified_reader(path)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 assert r(0x450435,4)==bytes.fromhex('c646020b')
 assert r(0x4383bf,3)==bytes.fromhex('0fb7c0')  #unsigned absolute delta includes32768
 assert struct.unpack('<d',r(0x7dccc8,8))[0]==0.15915497620314795
 assert struct.unpack('<d',r(0x7dccd0,8))[0]==65536.
 for address,value in [(0x7dcb64,10000.),(0x7dd568,.1),(0x7dd578,.7)]:
  assert r(address,4)==struct.pack('<f',value),hex(address)
 print('PC2004 homing evidence: PASS collection/range/order/ring-ID/pursuit/turn/RNG ranges and source bearing constants; static only')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: test_pc2004_homing_evidence.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
