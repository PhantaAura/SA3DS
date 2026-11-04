#!/usr/bin/env python3
"""Exact-owner flat task insertion/dispatch/free and trigger scheduling witnesses."""
from pathlib import Path
import argparse,hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
SPANS={
 (0x40b030,144):'b9619d64c2d6c8e586762e5c8fef324f7917ee2577cec63b543502c80beaf422',
 (0x40b0c0,112):'941d2292ed531870ef79c684ba36887033de6b89b9a1209664728a0b231f1377',
 (0x40b570,44):'8924af6bf257d2369341754e6d8f5f6128d385a51c1852039f4a6f9aba8f5672',
 (0x40b596,114):'f4243bbd5e09ad6a386e4596b1c30bf040eaaca134ea562d652ac0c00715689e',
 (0x40b68c,44):'9f6a5b3ba275d0f6040fb485d0da23899b0bc43b3551e35c193fcb962b32768c',
 (0x46bae0,48):'03dcff4ef3af26401e74b1a8e3249c65cd7bd3707515021d605c8387f347e4c7',
 (0x46bbeb,8):'fb71530b0b6c6a591d448971bf13408f062c37bb1e9ff9596226eac720976109',
 (0x4fbe40,32):'c64f0128a805f15495a2ab8fcdf53cfaf39037b8dedc3ca8c01fbcab42ba4615',
 (0x4fbce0,32):'91affabeec6922fd286b371860a2d3e5658fb9168b90f4eef8794c47f9f758cf',
 (0x4fbddc,64):'5402e2190b6c7d313b448dd932eada9cf93a332fec96d3095f803f1d11c7f778',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 assert r(0x40b0e0,2)==bytes.fromhex('8b1e') #next link captured before main
 assert r(0x40b109,2)==bytes.fromhex('ffd7') #dispatch captured current callback
 assert r(0x40b08f,2)==bytes.fromhex('8908') #new task's next=old head
 assert r(0x40b59a,6)==bytes.fromhex('8b5e04578b3e') #links read after destructor
 assert r(0x4fbe18,3)==bytes.fromhex('c60602') #action2 AFTER summon service call
 for site,target in ((0x4fbe0f,0x4f88b0),(0x40b576,0x40afb0)):
  assert r(site,1)==b'\xe8' and site+5+struct.unpack('<i',r(site+1,4))[0]==target
 print('Owned task/trigger owner witnesses: PASS exact SHA,10 spans,head insertion,next-link snapshot,live post-destructor links,LIFO free reuse,constructor callbacks and post-summon action store; native binding separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();verify(a.exe)
