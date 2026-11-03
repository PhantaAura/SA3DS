#!/usr/bin/env python3
"""Compare imported checkpoint callback matrices/material selectors to raw owner data."""
from pathlib import Path
import argparse,math,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
from tools.build_pc2004_checkpoint_assets import build
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.importers.pc2004_pvm_archive import PVMArchive

def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def identity():return [[float(i==j) for j in range(4)] for i in range(4)]
def mul(a,b):return [[f32(sum(a[i][k]*b[k][j] for k in range(4))) for j in range(4)] for i in range(4)]
def audit(exe,system):
 r=verified_reader(exe);lut=struct.unpack('<1025f',r(0x88af78,4100))
 def sample(n):
  n&=4095
  return lut[n] if n<1024 else lut[2048-n] if n<2048 else -lut[n-2048] if n<3072 else -lut[4096-n]
 def sine(n):
  n&=65535;a=sample(n>>4);return f32(a+(sample((n>>4)+1)-a)*((n&15)/16))
 def rotation(axis,n):
  m=identity();s=sine(n);c=sine(n+16384)
  if axis==0:m[1][1]=c;m[1][2]=-s;m[2][1]=s;m[2][2]=c
  if axis==1:m[0][0]=c;m[0][2]=s;m[2][0]=-s;m[2][2]=c
  if axis==2:m[0][0]=c;m[0][1]=-s;m[1][0]=s;m[1][1]=c
  return m
 rows=[]
 for act in (0,1):
  raw=read_source(system,f'SET010{act}S.BIN');count=struct.unpack_from('<I',raw)[0]
  for index in range(count):
   p=struct.unpack_from('<Hhhh6f',raw,32+index*32)
   if p[0]&4095==19:rows.append((act,index,p))
 positions=[struct.unpack('<3f',r(va+8,12)) for va in (0x8c908c,0x8c8004,0x8c76dc)]
 package,metadata=build(exe,system);assert metadata['counts_by_act']==[4,3]
 with tempfile.TemporaryDirectory(prefix='sa3ds-checkpoint-owner-') as tmp:
  pak=Path(tmp)/'checkpoint.sa3dspak';pak.write_bytes(package)
  output=subprocess.check_output(['bash',str(ROOT/'tests/run_checkpoint_assets.sh'),str(pak),'dump'],text=True,cwd=ROOT)
 actual={};textures={};rules={}
 for line in output.splitlines():
  fields=line.split()
  if fields[0]=='RULES':rules[int(fields[1])]=tuple(map(int,fields[2:]));continue
  if fields[0]=='TEXTURE':textures[int(fields[1])]=int(fields[2]);continue
  actual[tuple(map(int,fields[:3]))]=list(map(float,fields[3:]))
 assert r(0x44f5c7,1)==b"\x6a"
 checkpoint_category=r(0x44f5c8,1)[0]
 for i,category in enumerate((0,checkpoint_category)):
  expected=(r(0x90e3bc+category,1)[0],r(0x90e3b0+category,1)[0],r(0x90e3c8+category,1)[0],struct.unpack('<I',r(0x90e3d4+category*4,4))[0])
  assert rules[i]==expected,(category,rules[i],expected)
 pvm=PVMArchive(read_source(system,'OBJ_REGULAR.PVM'),'OBJ_REGULAR.PVM')
 for component in (1,2,3,4):assert textures[component]==pvm.entries[59 if component<3 else 60].global_index
 phases=((16384,-16384),(16384,-11833),(0,-16384),(9130,32767),(-16384,-32768),(125,-987))
 maximum=0.;checks=0
 for index,(_,_,p) in enumerate(rows):
  parent=identity()
  for i in range(3):parent[i][3]=p[4+i]
  for axis in (2,0,1):parent=mul(parent,rotation(axis,p[1+axis]))
  for phase,(px,py) in enumerate(phases):
   for component in range(3):
    expected=parent
    if component:
     translate=identity()
     for i in range(3):translate[i][3]=positions[component][i]
     expected=mul(parent,translate)
     expected=mul(expected,rotation(1,py if component==1 else -py))
     expected=mul(expected,rotation(0,px))
    got=actual[(index,phase,component)]
    for i in range(4):
     for j in range(4):
      error=abs(expected[i][j]-got[j*4+i]);maximum=max(maximum,error);checks+=1
      if error>0.000025:raise AssertionError((index,phase,component,i,j,expected[i][j],got[j*4+i],error))
 print(f'Checkpoint owner assets: PASS {len(rows)} original placements/{len(rows)*6} callback poses/{checks} matrix components, max_error={maximum:.9g},inactive59/active60 texture selectors; native gameplay/raster separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);args=ap.parse_args();audit(args.exe,args.system)
