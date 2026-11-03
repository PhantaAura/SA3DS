#!/usr/bin/env python3
"""Create private scattered-ring CCI/category7 data from the owner's exact EXE."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
from tools.verify_pc2004_ring_scatter import verify

def build(exe):
 verify(exe);r=verified_reader(exe)
 b=bytearray(72);b[:8]=b'SA3DSCAT';struct.pack_into('<II',b,8,1,0);b[16:64]=r(0x916b10,48)
 category=7
 struct.pack_into('<BBB x I',b,64,r(0x90e3bc+category,1)[0],r(0x90e3b0+category,1)[0],r(0x90e3c8+category,1)[0],struct.unpack('<I',r(0x90e3d4+category*4,4))[0])
 metadata={'source_profile':'PC2004-scattered-ring-CCI-v1','source_exe_sha256':PROFILE_SHA256,
           'scope':'private source916B10 CCI/category7 rules; task/art/audio remain separate'}
 return sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('SCATTER',b,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('exe',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
 output=args.output.resolve()
 if output.is_relative_to(ROOT):raise SystemExit('owner-derived package must be outside the source tree')
 data=build(args.exe);output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(data)
 print(f'Owner scatter package: {len(data)} bytes SHA256 {hashlib.sha256(data).hexdigest()}')
