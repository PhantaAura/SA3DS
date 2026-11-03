#!/usr/bin/env python3
"""Convert owner's checkpoint SET/CCI/Basic components into one external package."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_restart_record import verify
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_chrmodels import SonicDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_spring_visual import Adapter
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,read_source
from tools.build_pc2004_ring_assets import compact_textures

def build(exe,system):
 verify(exe);r=verified_reader(exe);d=PC2004StageDecoder(exe)
 decoder=SonicDecoder.__new__(SonicDecoder);decoder.pe=d.pe
 nodes=decoder.hierarchy(0x8c908c)
 if [n.va for n in nodes]!=[0x8c908c,0x8c8004,0x8c76dc]:raise ValueError('checkpoint hierarchy mismatch')
 if struct.unpack('<6I',r(0x916a38,24))!=(0x8c8004,0x44efe0,0x8c76dc,0x44efe0,0,0):raise ValueError('post callback mismatch')
 sources=[read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN')) for act in (0,1)]
 rows=[(s.act,i,p.raw) for s in sources for i,p in enumerate(s.records) if p.type_word&4095==19]
 if not rows or len(rows)>32:raise ValueError('checkpoint placement limit')
 co=64+len(rows)*40;po=co+48;ro=po+96;no=ro+16;land=no+36;lo=land+24;total=lo+4100
 config=bytearray(total)
 struct.pack_into('<8s9I2f',config,0,b'SA3CHKP\0',1,len(rows),64,co,po,ro,no,lo,total,360000.,360100.)
 struct.pack_into("<I",config,52,land)
 for i,row in enumerate(rows):struct.pack_into('<II32s',config,64+i*40,*row)
 config[co:co+48]=r(0x916a08,48);config[po:po+96]=r(0x91cbf0,96)
 for i,category in enumerate((0,3)):
  struct.pack_into('<BBBxI',config,ro+i*8,r(0x90e3bc+category,1)[0],r(0x90e3b0+category,1)[0],r(0x90e3c8+category,1)[0],struct.unpack('<I',r(0x90e3d4+category*4,4))[0])
 for i,n in enumerate(nodes):struct.pack_into('<3f',config,no+i*12,*n.position)
 config[land:land+24]=r(0x916a50,24)
 config[lo:]=r(0x88af78,4100)
 pvm=PVMArchive(read_source(system,'OBJ_REGULAR.PVM'),'OBJ_REGULAR.PVM');chunks=[];report=[]
 for index,(node,active) in enumerate(((0,0),(1,0),(2,0),(1,1),(2,1))):
  geometry,materials,meta=build_visual_chunk(Adapter(d,nodes[node].model_va),0,pvm.texture_count)
  if node:
   first=materials[0];materials[0]=(*first[:3],60 if active else 59,first[4])
  materials,textures,used=compact_textures(materials,pvm)
  payload=sa3dspak.build([('METADATA',b'{"scope":"owner checkpoint Basic component"}',0),('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)])
  chunks.append((f'VISUAL{index}',payload,0));report.append({'node':node,'active':active,'model':hex(nodes[node].model_va),'textures':used,'geometry':meta})
 metadata={'source_profile':'PC2004-checkpoint-v1','source_exe_sha256':d.sha256,'source_pvm_sha256':pvm.sha256(),
  'source_set_sha256':[s.sha256 for s in sources],'counts_by_act':[sum(row[0]==act for row in rows) for act in (0,1)],'components':report,
  'scope':'private original placement/CCI/player CCI/rules/trig/Basic art; task/post transforms/collision/respawn are runtime-owned'}
 data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('CHECKPOINT',config,0),*chunks]);sa3dspak.parse(data)
 return data,metadata
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
 if args.output.resolve().is_relative_to(ROOT):raise SystemExit('owner-derived package must remain outside source tree')
 data,report=build(args.exe,args.system);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
 print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'counts_by_act':report['counts_by_act']}))
