#!/usr/bin/env python3
"""Exact-owner SET clip/unload and independent camera-reference witnesses."""
from pathlib import Path
import argparse,hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
SPANS={
 (0x46b750,88):'a33ac8b39b2e75c5425effe386ba0bfaf43bdc35db21c56aa6cf1e348072d08a',
 (0x46c010,211):'c666a2adba947a9531936d0a463fab99f6e48d2671d4930167acd7f0f6d0ea5b',
 (0x46c360,40):'d6311165e3ced185c5e1a1dece1e0f9622de19fc23e5eac2e31efcc221953788',
 (0x46bcad,16):'7d400ab7ee5befa217bddfa0cff5a9745551d61a239b9c0be5554f6a644a559d',
 (0x43820e,44):'f8ff59ae57b43483718900d6c66add58df7ea86be669f20260eb8e914d0a9ab8',
 (0x5022c0,103):'a6f6fd3232d516884130b5f54b8a79fec54213ecb37c2f87eb7d7cb8a3fa0216',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 assert r(0x7e05e0,4)==struct.pack('<f',100)
 assert r(0x46c379,5)==b'\x68'+struct.pack('<f',168100)
 assert r(0x46c0b8,5)==b'\x68'+struct.pack('<f',1600)
 assert r(0x46c0d2,7)==bytes.fromhex('c7451070b54000') #deferred main replacement
 assert r(0x438221,6)==bytes.fromhex('892db0cbb203') #camera task initialization
 assert r(0x5022c0,5)==bytes.fromhex('a1b0cbb203') #whale far check: camera,notplayer
 for kind in (67,68):assert r(0x1036420+kind*20+4,4)==struct.pack('<f',160000)
 print('SET clip owner witnesses: PASS exact SHA,6 spans,inclusive range,SET load+100,default168100,player1-character/fixed1600,deferred40B570 and separate whale camera reference; native task scheduling separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();verify(a.exe)
