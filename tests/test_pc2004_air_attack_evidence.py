#!/usr/bin/env python3
"""Exact owner instruction witnesses for ordinary airborne attack recovery."""
from pathlib import Path
import hashlib,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x43bf40,0x47):'1ac38b4e3325b4fdd74cdf9705fbf5a97a1827639f611d549cb199e59923c1b5',
 (0x494b80,0xe0):'b080d105ff0a6636e0507f839feccc41890e4ec7720e49275fe89bcd71faffdb',
 (0x492300,0x410):'f0dfac60151d1eebc1fe5834ec1fd8402e53b187b289164b7a8dc533c8582c71',
 (0x497b56,0x18e):'b00faf2249ba2775cd660e8fafc4df5dba33b6773c25573dc2f3ff0ebc5bdc54',
}
def verify(path):
 r=verified_reader(path)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 assert struct.unpack('<I',r(0x49a850+13*4,4))[0]==0x497b56
 assert struct.unpack('<I',r(0x49bfbc+14*4,4))[0]==0x49afac
 assert r(0x497b81,4)==bytes.fromhex('663d6801')
 assert r(0x497b88,7)==bytes.fromhex('66899382000000')
 assert r(0x497bb9,7)==bytes.fromhex('0fbf8380000000')
 assert struct.unpack('<f',r(0x7df1a8,4))[0]==6.
 assert r(0x7dfff0,4)==struct.pack('<f',.9)
 for address,target in [(0x492350,0x44c270),(0x492358,0x443f50)]:
  ins=r(address,5);assert ins[0]==0xe8 and address+5+int.from_bytes(ins[1:],'little',signed=True)==target
 print('PC2004 air attack evidence: PASS jump-entry/init/selector/decay ranges and direct action14 movement calls; static only')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: test_pc2004_air_attack_evidence.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
