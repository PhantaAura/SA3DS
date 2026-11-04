#!/usr/bin/env python3
"""Extract owner's whale CCI records/rules into an external package."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
SPANS={
 (0x502a9e,116):'e58cf55edee23f3368bacac89aa3ef4b5bc876e6550797166a7a9e3efa527913',
 (0x502906,37):'e034eddda4b4c89fa452fcea2b0e6a5e225c91c7c59cbb5fe99f27c66c84fce7',
 (0x502357,23):'d0e6d924b2746dcb654d34960b5e0cfca0829b818c25aefd5016b7ee3443c4bd',
 (0x419160,272):'961eb26b4a65107a5600a03f6364c01b75b0708c90da76ca04497f78ce56e2e1',
 (0x41caf0,201):'9af66e11cb385c32b9e16587c54b283db08bb9c7be34fcec025e65d74895ac93',
 (0x41c530,15):'6f41fea6ab149abb35220f79df35c9c2943d101a98e59d22c07bb98c61b8308a',
 (0x10c8790,96):'fa6065750c4e18e3258102eed733333d5aa77585ea9514edfd594f21444df09f',
 (0x90e3b0,76):'4baf3ec2fe815df77ad0a6ab34b8a01f6ae43cc4b690ac6d59bb3681a2e3c3f7',
}
def build(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 assert r(0x502a9e,4)==b'\x6a\x02\x6a\x02'
 assert r(0x502aa4,5)==b'\x68'+struct.pack('<I',0x10c8790)
 assert r(0x502aeb,4)==bytes.fromhex('83483410')
 assert r(0x50235b,4)==bytes.fromhex('836034ef')
 category=r(0x502a9f,1)[0]
 rules=[r(a+category,1)[0] for a in (0x90e3bc,0x90e3b0,0x90e3c8)]
 excluded=struct.unpack('<I',r(0x90e3d4+category*4,4))[0]
 broad=struct.unpack('<f',r(0x502adb,4))[0]
 payload=struct.pack('<8s6I4BIfI',b'SA3WCCL\0',1,2,48,48,144,0,category,*rules,excluded,broad,0)+r(0x10c8790,96)
 assert len(payload)==144
 meta={'source_exe_sha256':PROFILE_SHA256,'scope':'two whale CCI spheres/category rules; task origin and animated tip remain separate'}
 return sa3dspak.build([('METADATA',json.dumps(meta,sort_keys=True).encode(),0),('WHALE_CCL',payload,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data=build(a.exe);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Whale CCL private package {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}; {len(SPANS)} owner spans')
