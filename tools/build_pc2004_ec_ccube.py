#!/usr/bin/env python3
"""Owner-only Emerald Coast C CUBE package: SET rows and the original form3 CCI."""
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
def build(exe,system):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 entry=struct.unpack('<BBHfIII',r(0x1036420+24*20,20));assert entry[:4]==(2,2,1,360000.) and entry[5]==0x4d47e0
 assert r(0x4d4827,9)==bytes.fromhex('6a046a0168e8e79b00') #41CAF0(task,9BE7E8,1,category4)
 cci=r(0x9be7e8,48);assert cci[1]==3 and cci[2]==0x77 and struct.unpack_from('<3f',cci,8)==(0.,0.,0.)
 rows=[]
 for act in (0,1,2):
  blob=read_source(system,f'SET010{act}S.BIN');n=struct.unpack_from('<I',blob,0)[0]
  for i in range(n):
   t,rx,ry,rz,x,y,z,sx,sy,sz=struct.unpack_from('<Hhhhffffff',blob,32+i*32)
   if t&0xfff==24:rows.append((act,i,t,rx,ry,rz,x,y,z,sx,sy,sz))
 head=struct.Struct('<8s6If');row=struct.Struct('<IIH3hxx3f3f')
 cfg=bytearray(head.size+48+len(rows)*row.size)
 head.pack_into(cfg,0,b'SA3CUBE\0',1,len(rows),head.size+48,row.size,4,len(cfg),360000.)
 cfg[head.size:head.size+48]=cci
 for i,v in enumerate(rows):row.pack_into(cfg,head.size+48+i*row.size,*v)
 meta={'source_exe_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'rows':len(rows),'scope':'C CUBE SET rows + raw CCI; dims are scale+10 at runtime'}
 data=sa3dspak.build([('METADATA',json.dumps(meta).encode(),0),('CCUBE',bytes(cfg),0)]);sa3dspak.parse(data);return data,len(rows)
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data,n=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'C CUBE owner package: PASS 2 spans,{n} rows,form3/category4 CCI; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
