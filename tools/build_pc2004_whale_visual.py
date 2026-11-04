#!/usr/bin/env python3
"""Owner-only whale Basic visual, original shape banks and explicit UV-seam map."""
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
from tools.build_pc2004_whale_shape import build as build_shape
class WhaleAdapter(Adapter):
 def landtable(self,*args):return SimpleNamespace(texture_archive='OBJ_BEACH')
def build(exe,system):
 r=verified_reader(exe);d=PC2004StageDecoder(exe)
 assert r(0x420fc0,5)==bytes.fromhex('a1b490b203')
 table=struct.unpack('<I',r(0x90eb68+4,4))[0];name,tex=struct.unpack('<2I',r(table,8))
 assert r(name,10)==b'OBJ_BEACH\0' and d.resolve_texture_archive(tex)=='OBJ_BEACH'
 model=d.model(0x106bb20);assert model.point_count==238
 pvm=PVMArchive(read_source(system,'OBJ_BEACH.PVM'),'OBJ_BEACH.PVM')
 mapping=[];geometry,materials,meta=build_visual_chunk(WhaleAdapter(d,model.va),0,pvm.texture_count,mapping)
 assert mapping and all(a==model.va and 0<=i<238 for a,i in mapping)
 #Explicit original index, never nearest-position matching: coincident source
 #vertices may have different shape trajectories.
 payload=struct.pack('<8s6I',b'SA3WMAP\0',1,238,len(mapping),32,2,32+len(mapping)*2)+struct.pack('<'+'H'*len(mapping),*(i for _,i in mapping))
 materials,textures,used=compact_textures(materials,pvm)
 visual=sa3dspak.build([('METADATA',b'{"scope":"original whale Basic resource at identity"}',0),('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)])
 metadata={'source_exe_sha256':d.sha256,'source_pvm_sha256':pvm.sha256(),'source_texture_slots':used,'geometry':meta,'scope':'original Basic resource/shape/source-vertex map; actor transforms and native event execution separate'}
 return sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('WHALE_MAP',payload,0),('WHALE_SHAPE',build_shape(exe),0),('VISUAL',visual,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Whale visual private package{len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
