#!/usr/bin/env python3
"""Owner-only Emerald Coast invisible C CUBE or WALL package: SET rows and raw CCI."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
from tools.build_pc2004_emerald_coast_visual import read_source
SPANS={
 (0x4d47e0,102):'137fbd9dee50249a376ab7fcb3c7a5fac0b205adf9465e6be11f18723bd52f9a', #main/constructor
 (0x4d46c0,64):'dc547b563d96b1556d3ed35dc5f4e2fbd236e9a73a5cd35e32cbfa75c0168702', #scale+10 dimensions
}
def build(exe,system,*,wall=False):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 kind,category,form,task,cci_va=(20,8,9,0x4d4850,0x9be818) if wall else (24,4,3,0x4d47e0,0x9be7e8)
 entry=struct.unpack('<BBHfIII',r(0x1036420+kind*20,20));assert entry[:4]==(2,3 if wall else 2,1,360000.) and entry[5]==task
 assert r(0x4d4897 if wall else 0x4d4827,9)==bytes([0x6a,category,0x6a,1,0x68])+struct.pack('<I',cci_va) #41CAF0(task,CCI,1,category)
 cci=r(cci_va,48);assert cci[1]==form and cci[2]==0x77 and struct.unpack_from('<3f',cci,8)==(0.,0.,0.)
 if wall:
  assert struct.unpack('<I',r(0x90e3d4+8*4,4))[0]==0
  assert cci[4:8]==bytes(4) and cci[36:48]==bytes(12)
 rows=[]
 for act in (0,1,2):
  blob=read_source(system,f'SET010{act}S.BIN');n=struct.unpack_from('<I',blob,0)[0]
  for i in range(n):
   t,rx,ry,rz,x,y,z,sx,sy,sz=struct.unpack_from('<Hhhhffffff',blob,32+i*32)
   if t&0xfff==kind:rows.append((act,i,t,rx,ry,rz,x,y,z,sx,sy,sz))
 head=struct.Struct('<8s6If');row=struct.Struct('<IIH3hxx3f3f')
 cfg=bytearray(head.size+48+len(rows)*row.size)
 head.pack_into(cfg,0,b'SA3WALL\0' if wall else b'SA3CUBE\0',1,len(rows),head.size+48,row.size,category,len(cfg),360000.)
 cfg[head.size:head.size+48]=cci
 for i,v in enumerate(rows):row.pack_into(cfg,head.size+48+i*row.size,*v)
 meta={'source_exe_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'rows':len(rows),'kind':kind,'category':category,'scope':'owner invisible CCL SET rows + raw CCI; dims are scale+10 at runtime'}
 data=sa3dspak.build([('METADATA',json.dumps(meta).encode(),0),('WALL' if wall else 'CCUBE',bytes(cfg),0)]);sa3dspak.parse(data);return data,len(rows)
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);ap.add_argument('--wall',action='store_true',help='registry20 form9/category8 WALL instead of C CUBE');a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data,n=build(a.exe,a.system,wall=a.wall);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Invisible CCL owner package: PASS 2 spans,{n} rows,kind{20 if a.wall else 24} CCI; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
