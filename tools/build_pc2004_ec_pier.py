#!/usr/bin/env python3
"""Owner-only Emerald Coast O PIER package: SET rows, dynamic land quad, intact visual."""
from pathlib import Path
from types import SimpleNamespace
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_spring_visual import Adapter
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,read_source
from tools.build_pc2004_ring_assets import compact_textures
SPANS={
 (0x4ff060,219):'b15bbf5454de3baee9b973d8bd045a44eac5b410a6291a9bbf308affff1a5403', #constructor
 (0x4fed30,815):'da03e26260e227cc432949829a395c625ac29d1114f8e7357779140a4eb15d8e', #main
 (0x4fe8e0,130):'340e5600545eae81a68c37be72566aea3aee3bc44a68ae33f88c009b54a97a28', #display
 (0x4fe970,52):'0e4fb298f13ccda702570d93691465a9e89fa6214ba7080e5f396bfd66e06468', #destructor
 (0x43b2e0,80):'1ca1fb4386b7c9387cc71abe866e85b00c209019cb4dca78759e70bb66e667da', #DynamicCOL add
 (0x441c30,60):'f20315bc85b595daae42687bf079099a10cf2e0dad0642e06b1828d30ea472b9', #standing-on task
}
SURFACE_FLAGS=0x68000001
class PierAdapter(Adapter):
 def landtable(self,*args):return SimpleNamespace(texture_archive='OBJ_BEACH')
def set_rows(system,act):
 blob=read_source(system,f'SET010{act}S.BIN');n=struct.unpack_from('<I',blob,0)[0]
 if len(blob)!=32+32*n:raise ValueError('SET size')
 rows=[]
 for i in range(n):
  t,rx,ry,rz,x,y,z,sx,sy,sz=struct.unpack_from('<Hhhhffffff',blob,32+i*32)
  if t&0xfff!=59:continue
  #4FF071: ftol(scale.x) & 0x80000001 signed parity selects variant byte.
  v=int(sx);v=v%2 if v>=0 else -((-v)%2)
  if v<0 or rx or rz:raise ValueError('unsupported pier variant/rotation profile')
  rows.append((act,i,t,v,rx,ry,rz,x,y,z,sx,sy,sz))
 return rows
def build(exe,system):
 r=verified_reader(exe);d=PC2004StageDecoder(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 entry=struct.unpack('<BBHfIII',r(0x1036420+59*20,20))
 assert entry[:4]==(6,3,1,250000.) and entry[5]==0x4ff060
 assert r(0x4fed3c,5)==bytes.fromhex('6800247448') #46C010 threshold250000
 assert r(0x4ff0f1,5)==bytes.fromhex('6801000068') #DynamicCOL flags68000001
 assert r(0x4fedaa,6)==bytes.fromhex('66817e062c01') #broken timer300
 assert r(0x4fef39,3)==bytes.fromhex('894314') #break clears task display
 break_d2=struct.unpack('<f',r(0x7ec96c,4))[0];assert break_d2==5625.
 model_va=struct.unpack('<I',r(0x1091ef0,4))[0];collision=d.model(model_va)
 points=d.model_points(collision);tris=d.model_triangles(collision)
 assert len(points)==4 and len(tris)==2
 table=struct.unpack('<I',r(0x90eb68+4,4))[0];name,tex=struct.unpack('<2I',r(table,8))
 assert r(name,10)==b'OBJ_BEACH\0' and d.resolve_texture_archive(tex)=='OBJ_BEACH'
 rows=set_rows(system,0)+set_rows(system,1)
 assert len(rows)==62 and {row[3] for row in rows}=={0}
 pvm=PVMArchive(read_source(system,'OBJ_BEACH.PVM'),'OBJ_BEACH.PVM')
 visuals=[];report=[]
 for variant in (0,1):
  objects=struct.unpack('<I',r(0x10c80c4+4*variant,4))[0];intact=d.object(struct.unpack('<I',r(objects,4))[0])
  #Eval flags UNIT_POS|UNIT_ANG|UNIT_SCL make the node an identity transform
  #(variant0 stores a -8e-6 Z that Ninja therefore ignores).
  if intact.child_va or intact.sibling_va or (intact.flags&7)!=7:
   raise ValueError('intact pier object is not an identity leaf')
  geometry,materials,meta=build_visual_chunk(PierAdapter(d,intact.model_va),0,pvm.texture_count)
  materials,textures,used=compact_textures(materials,pvm)
  visuals.append((f'VISUAL{variant}',sa3dspak.build([('METADATA',b'{"scope":"original intact O PIER"}',0),
   ('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)]),0))
  report.append({'variant':variant,'model':hex(intact.model_va),'textures':used,'geometry':meta,
   'pieces':struct.unpack('<I',r(0x10c80bc+4*variant,4))[0]})
 header=struct.Struct('<8s10I4fI')
 row=struct.Struct('<IIHBx3hxx3f3fI')
 row_off=header.size;point_off=row_off+len(rows)*row.size;index_off=point_off+len(points)*12
 total=index_off+len(tris)*6
 config=bytearray(total)
 header.pack_into(config,0,b'SA3PIER\0',1,len(rows),row_off,row.size,len(points),point_off,len(tris)*3,index_off,total,SURFACE_FLAGS,
  break_d2,250000.,250000.,50.,300)
 for i,(act,src,t,v,rx,ry,rz,x,y,z,sx,sy,sz) in enumerate(rows):
  row.pack_into(config,row_off+i*row.size,act,src,t,v,rx,ry,rz,x,y,z,sx,sy,sz,0)
 for i,p in enumerate(points):struct.pack_into('<3f',config,point_off+i*12,*p)
 for i,tri in enumerate(tris):struct.pack_into('<3H',config,index_off+i*6,*tri)
 metadata={'source_exe_sha256':d.sha256,'source_pvm_sha256':pvm.sha256(),'rows':len(rows),'collision_model':hex(model_va),
  'variants':report,'scope':'O PIER intact profile: SET rows, DynamicCOL quad, intact Basic visual; pieces/throw/sound/railing CCL separate'}
 #4FF10C: 41CAF0(task, CCI 10C8190[variant], count2, category4) railing boxes.
 assert r(0x4ff113,4)==bytes.fromhex('6a046a02')
 cci=b''
 for variant in (0,1):
  table=struct.unpack('<I',r(0x10c8190+4*variant,4))[0];raw=r(table,96)
  for k in range(2):assert raw[k*48+1]==3 and raw[k*48+2]==0x77 #form3 box, push77
  cci+=raw
 #4FEFD6..: pieces 1..count-1 of objtab[variant0] start at pier pos + object
 #offset (unrotated) and draw only their model (4FB2A0 T*Rx*Ry, 407970).
 objects=struct.unpack('<I',r(0x10c80c4,4))[0];count=struct.unpack('<I',r(0x10c80bc,4))[0]
 assert 2<=count<=33 and r(0x4febc0,4)==bytes.fromhex('83ec0853')
 meta=bytearray(struct.pack('<8s2I',b'SA3PPCS\0',1,count-1));pieces=[]
 for i in range(1,count):
  ob=d.object(struct.unpack('<I',r(objects+4*i,4))[0])
  if ob.child_va or ob.sibling_va or not ob.model_va:raise ValueError('pier piece is not a leaf model')
  meta+=struct.pack('<3f',*ob.position)
  g,m,_=build_visual_chunk(PierAdapter(d,ob.model_va),0,pvm.texture_count);m,t,_=compact_textures(m,pvm)
  pieces.append((f'PIECE{i-1:02d}',sa3dspak.build([('METADATA',b'{"scope":"original O PIER piece model"}',0),
   ('GEOMETRY',g,0),('MATERIALS',build_material_chunk(m),0),('TEXTURES',t,0)]),0))
 data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('PIER',bytes(config),0),('PIER_CCL',cci,0),
  ('PIER_PIECES',bytes(meta),0),*pieces,*visuals])
 sa3dspak.parse(data);return data
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'O PIER owner package: PASS 6 spans,62 act0 rows,DynamicCOL quad,2 intact visuals; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
