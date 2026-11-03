#!/usr/bin/env python3
"""Static witnesses for ordinary death/task order/action-stage restart branches."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x42c350,400): 'c75782510eb8749577b40e4ef1c5fca006f9397561d767e1094d742251439a1b',
 (0x42bf90,32): 'd05784205d127252c8d7eda2c8047929796531020838c271976b955ae012d2d8',
 (0x40ef40,32):'cfac35e2cf279d1a8c5254b250bfbf57c82900fcbe1c45dd9c1fe355c57537d5',
 (0x440c80,80):'766583051c1afc3fbf407b4903b7eea7496ee559914272b0b89c2b19aee32909',
 (0x40b030,144):'b9619d64c2d6c8e586762e5c8fef324f7917ee2577cec63b543502c80beaf422',
 (0x40b0c0,112):'941d2292ed531870ef79c684ba36887033de6b89b9a1209664728a0b231f1377',
 (0x49676b,79):'57022f3a7170ae5baf919919e9e741413035a66783150e542ff30d9a6f3e5ddf',
 (0x49bdb4,60):'5f16d402b7daf1f8b251a9e3f7caef907787ad9b80d22e2d96e77af3ad86f788',
 (0x42bef0,128):'736ed22e093849b0c886165ba5b6eb7fd2931044ed92d9089a4d0716b5bd9542',
 (0x42bf70,32):'96f80a14254f4148b1da69a1b20f81bdf6ff1ac536c77bd67216933f69f0808e',
 (0x42c570,192):'42285c195df27f61c45020e97080f99a8cb02d4bae4d16d776b4835d1eca4e0d',
 (0x42c630,32):'cb71a133beb2d6d9b056b861767e9a1ab5756d1517959fb83c1f2ad55f67dda9',
 (0x42c650,224):'991c7b1ed50b6e971907a814e31efe0cbee71defd6c21e542f10006f60a144cd',
 (0x425af0,16):'3a2b65089b138e4af25df0f57571ad31449b6e0b7fcbc43f9e376640076b714f',
 (0x425b60,128):'b2ca77f5e0b672e1dc540706de5377a3b163649061bd7e90225afa0ca09bdb4c',
 (0x416610,3220):'4c1ef5c28dfbe460d40cdc63ba4efc1dc11bc4fd916108ac1fcea47ab3987e60',
}
def call(r,a,target):
 assert r(a,1)==b'\xe8',hex(a)
 assert a+5+struct.unpack('<i',r(a+1,4))[0]==target,hex(a)
def verify(exe):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 for case,target in [(2,0x41676b),(3,0x41677e),(4,0x4167a0),(6,0x416e07),(8,0x416f5f),(15,0x41680c)]:
  assert struct.unpack('<I',r(0x417314+case*4,4))[0]==target
 assert struct.unpack('<I',r(0x49a850+85*4,4))[0]==0x49a848
 assert struct.unpack('<I',r(0x49bfbc+86*4,4))[0]==0x49bdb4
 assert r(0x440c8a,3)==bytes.fromhex('83f978') and r(0x440c92,1)==b'\x76' #old unsigned <=120
 assert r(0x49676b,3)==bytes.fromhex('c60756') #command50 action86
 assert r(0x49bdba,9)==bytes.fromhex('66c785240100005700') #incoming ground requests87
 for a,target in [(0x440c96,0x413c90),(0x440cac,0x424830),(0x440cb2,0x40b6c0),
                  (0x49bdc6,0x443ad0),(0x49bdce,0x448e50),(0x49bdd6,0x443f50),
                  (0x49bdde,0x44cdf0),(0x49bde6,0x43ee70)]:call(r,a,target)
 print('Death owner witnesses: PASS exact SHA,16 code spans,task snapshot/head order,old-counter branch,command86/ground87,action-stage dispatcher/faders/lives; static evidence only')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_player_death.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
