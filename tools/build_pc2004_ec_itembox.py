#!/usr/bin/env python3
"""Owner-only Emerald Coast O ITEMBOX package: SET rows, intact/broken CCI and
the original box and item-icon geometry (OBJ_REGULAR)."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_spring_visual import Adapter
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,read_source
from tools.build_pc2004_ring_assets import compact_textures
from tools.verify_pc2004_item_box import verify as verify_rules
PARTS=(0x8bf260,0x8beaa8,0x8be620);ICON=0x8be270
def rows(system):
 out=[]
 for act in (0,1):
  b=read_source(system,f'SET010{act}S.BIN');n=struct.unpack_from('<I',b,0)[0]
  for i in range(n):
   t,rx,ry,rz,x,y,z,sx,sy,sz=struct.unpack_from('<Hhhhffffff',b,32+i*32)
   if t&0xfff!=13:continue
   k=int(sx);k=8 if k<0 else (0 if k>8 else k)
   out.append((act,i,t,k,rx,ry,rz,x,y,z,sx,sy,sz))
 return out
def build(exe,system):
 verify_rules(exe);r=verified_reader(exe);d=PC2004StageDecoder(exe)
 for a,v in ((0x4d690e,'be70e28b00'),(0x4d6953,'6860f28b00'),(0x4d695f,'68a8ea8b00'),(0x4d696b,'6820e68b00')):
  assert r(a,5)==bytes.fromhex(v),hex(a)
 assert d.resolve_texture_archive(0x8B1CD8)=='OBJ_REGULAR'
 cci=r(0x9bf0c0,96)+r(0x9bf120,96)
 assert r(0x4d6ef5,4)==bytes.fromhex('6a046a02') #broken 41CAF0(task,9BF120,2,4)
 rules=b''.join(struct.pack('<BBBxI',r(0x90e3bc+c,1)[0],r(0x90e3b0+c,1)[0],r(0x90e3c8+c,1)[0],struct.unpack('<I',r(0x90e3d4+c*4,4))[0]) for c in (2,4)) #41CAF0 categories 2 (intact) / 4 (broken)
 assert cci[1]==1 and cci[49]==0 and struct.unpack_from('<4f',cci,20)[:2]==(5.,5.)
 rs=rows(system);assert len(rs)==18 and {x[3] for x in rs}=={2,3,4,6}
 pvm=PVMArchive(read_source(system,'OBJ_REGULAR.PVM'),'OBJ_REGULAR.PVM')
 head=struct.Struct('<8s6If');row=struct.Struct('<IIHBx3hxx3f3f')
 entry=struct.unpack('<BBHfIII',r(0x1036420+13*20,20));assert entry[:4]==(2,3,1,160000.)
 blob=bytearray(head.size+len(cci)+len(rs)*row.size)
 head.pack_into(blob,0,b'SA3IBOX\0',1,len(rs),head.size+len(cci),row.size,len(cci),len(blob),entry[3])
 blob[head.size:head.size+len(cci)]=cci
 for i,v in enumerate(rs):row.pack_into(blob,head.size+len(cci)+i*row.size,*v)
 chunks=[('ITEMBOX',bytes(blob),0),('IBOXRULE',rules,0)];report=[]
 def visual(model,key,texture=None):
  g,m,meta=build_visual_chunk(Adapter(d,model),0,pvm.texture_count)
  if texture is not None:m=[(*x[:3],texture,x[4]) for x in m] #4D6926: icon material texture = 0x9BF190[kind]
  m,t,used=compact_textures(m,pvm)
  chunks.append((key,sa3dspak.build([('METADATA',b'{"scope":"original O ITEMBOX geometry"}',0),('GEOMETRY',g,0),
   ('MATERIALS',build_material_chunk(m),0),('TEXTURES',t,0)]),0));report.append({'key':key,'model':hex(model),'textures':used})
 for i,model in enumerate(PARTS):visual(model,f'VISUAL{i}')
 for kind in (2,3,4,6):
  tex=struct.unpack('<I',r(0x9bf190+kind*8,4))[0]
  if kind==6:tex=struct.unpack('<I',r(0x9bf1d8,4))[0] #4D6835: Sonic (character0) sets the life icon
  visual(ICON,f'ICON{kind}',tex)
 meta={'source_exe_sha256':d.sha256,'source_pvm_sha256':pvm.sha256(),'rows':len(rs),'visuals':report,
  'scope':'O ITEMBOX rows, CCI 9BF0C0/9BF120, box parts 8BF260/8BEAA8/8BE620, icon 8BE270'}
 data=sa3dspak.build([('METADATA',json.dumps(meta,sort_keys=True).encode(),0)]+chunks);sa3dspak.parse(data)
 return data,rs
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data,rs=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'O ITEMBOX owner package: PASS {len(rs)} rows (kinds 2/3/4/6), CCI intact+broken, 3 box parts + 4 icon variants; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
