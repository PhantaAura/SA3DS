#!/usr/bin/env python3
"""Compare native API-state output with independently addressed owner records."""
from pathlib import Path
import argparse,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
from tools.build_pc2004_lighting_assets import build
from tools import sa3dspak

def audit(exe):
 r=verified_reader(exe);data=build(exe)
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-light-') as directory:
  root=Path(directory);good=root/'owner.sa3dspak';good.write_bytes(data)
  parsed=sa3dspak.parse(data);c=next(x for x in parsed['chunks'] if x['type']=='LIGHTING')
  #Use the archive's actual content range, not assumed file padding.
  payload=bytearray(data[c['offset']:c['offset']+c['size']]);bad=[]
  for name,offset,value in [('version',8,struct.pack('<I',2)),('nan',88,struct.pack('<I',0x7fc00000)),('profile',80,b'\x02')]:
   b=payload.copy();b[offset:offset+len(value)]=value;p=root/(name+'.sa3dspak')
   p.write_bytes(sa3dspak.build([('METADATA',b'{}',0),('LIGHTING',b,0)]));bad.append(p)
  output=subprocess.check_output(['bash',str(ROOT/'tests/run_pc2004_lighting.sh'),str(good),*map(str,bad)],text=True)
  rows=[line.split() for line in output.splitlines() if line.startswith(('0 ','1 '))]
  assert len(rows)==2
  #40AA30 normal EC row3 is selected for act0 and act1 fallback.
  p=r(0x903e88+3*96,96);f=lambda offset:struct.unpack_from('<f',p,offset)[0]
  expected=[f(16)]*3+[1]+[f(20),f(24),f(28),0]+[f(52),f(56),f(60),0]+[0]*3+[f(4),f(8),f(12)]+[0]*7
  for act,row in enumerate(rows):
   assert int(row[0])==act and int(row[1])==3
   actual=list(map(float,row[2:]));assert struct.pack('<25f',*actual)==struct.pack('<25f',*expected)
 print('Lighting owner API oracle: PASS both acts,all104 light bytes,strict version/NaN/profile rejection; API state only,not retail execution/shading')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();audit(a.exe)
