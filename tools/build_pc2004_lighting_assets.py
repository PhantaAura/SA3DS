#!/usr/bin/env python3
"""Owner-only ordinary Emerald Coast/Sonic lighting profile package."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader

def character_record(rows,level,act,kind=0):
 for stage,subact in ((level,act),(level,0),(0,0)):
  for row in rows:
   if row[0]==255:break
   if row[0]==stage and row[1]==subact and row[2]==kind:return row
 raise ValueError('no ordinary character profile')
def build(exe):
 from tools.verify_pc2004_lighting import verify
 verify(exe);r=verified_reader(exe)
 rows=[r(0x903e88+i*96,96) for i in range(255)]
 #40A8A0 initial default snapshot; no runtime SET/event stage-light edits claimed.
 stage=r(0x9008b4,48)
 first=r(0x900e88,48)
 if first[0]==0:stage=bytes((0,0,0,1))+first[4:]
 profiles=[character_record(rows,1,act) for act in (0,1)]
 if any(p[:4]!=bytes((1,0,0,3)) for p in profiles):raise ValueError('unexpected ordinary EC profile; re-audit source')
 payload=struct.pack('<8s6I',b'SA3LITE\0',1,2,32,80,176,272)+stage+b''.join(profiles)
 metadata={'scope':'ordinary EC Sonic mode2 API state,not full lighting/render state','source_exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'profile_rows':[next(i for i,row in enumerate(rows) if row==p) for p in profiles]}
 return sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('LIGHTING',payload,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise SystemExit('owner-derived lighting must stay outside source tree')
 data=build(a.exe);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}))
