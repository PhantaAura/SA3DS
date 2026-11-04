#!/usr/bin/env python3
"""Exact-owner task deletion, SET association and shared AO lifetime witnesses."""
from pathlib import Path
import argparse,hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
SPANS={
 (0x40b570,44):'8924af6bf257d2369341754e6d8f5f6128d385a51c1852039f4a6f9aba8f5672',
 (0x40b678,20):'f19bdc8254e8d5c0582ee15844fc9812432709812d9181f49bebf32813630ab7',
 (0x40b6c0,27):'32c10741c81a6f634332cbfda250f11a3cecf6eeb67e7a9bbf8d00d727d57b30',
 (0x46bae0,25):'0a02ae10fb5e4d72a732edc7f1a0aaec774bd52d08eea22eeb9bcfe1e2331602',
 (0x46bbeb,8):'fb71530b0b6c6a591d448971bf13408f062c37bb1e9ff9596226eac720976109',
 (0x46bcad,16):'7d400ab7ee5befa217bddfa0cff5a9745551d61a239b9c0be5554f6a644a559d',
 (0x4f88b0,87):'e58ee5564d1978f80fc7d35915acd04f54866b72043b22e36d8831509080ee39',
 (0x4f8910,23):'c8375b4bac0407adf3ae494cf28c37798149e2479b1718d04ed30f1fbd337b2e',
 (0x4f8930,52):'a5f94c3f9d807a237251d1739db847c8adf092363d2511f47670f8cac304bd3b',
 (0x5022a0,23):'c8d8d1837bfbe9f8d8bfacaafad5c006d814b0df6f14da113798cc9e3cd64dbd',
 (0x40b860,213):'25f15490017cb13f14c0295057370f03f0739414b9129402640239b4ed3aa960',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 for site,target in ((0x40b576,0x40afb0),(0x4f88ba,0x40b6c0),(0x4f88d5,0x40b860),(0x5022a5,0x4f8930),(0x5022ae,0x4bcbc0)):
  assert r(site,1)==b'\xe8' and site+5+struct.unpack('<i',r(site+1,4))[0]==target,hex(site)
 assert r(0x40b685,4)==bytes.fromhex('806002fe') #clear active bit only
 assert r(0x40b6d2,7)==bytes.fromhex('c7461070b54000') #mark deferred free
 assert r(0x46bbeb,5)==bytes.fromhex('804e0201fe') #active bit then byte counter INC
 assert r(0x4f8935,6)==bytes.fromhex('3b4424047528') #destroy only matching AO
 print('SET/whale lifetime owner witnesses: PASS exact SHA,11 spans,deferred40B6C0,registration/destructor order,SET flags/counter,AO replacement/conditional cleanup and RumbleA call; native task execution separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();verify(a.exe)
