#!/usr/bin/env python3
"""Convert six original spring Basic models to a bounded external owner package."""
from pathlib import Path
from types import SimpleNamespace
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.importers.pc2004_stage import PC2004StageDecoder,ObjectRecord,CollisionRecord
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,read_source
from tools.build_pc2004_ring_assets import compact_textures
# Original display routines7A4980/7A4AE0 draw three independent Basic models.
# The latter two source objects supply position/model only; rotation/scale
# are applied explicitly by the display callback, not by object traversal.
BASES=(0x8B679C,0x8B5C10)
OBJECTS=((0x8B649C,0x8B603C),(0x8B5924,0x8B54C4))
class Adapter:
    def __init__(self,dec,model):
        self.dec=dec;self.m=dec.model(model)
        self.row=CollisionRecord(0,self.m.center,self.m.radius,0,0,0,0,0x80000000)
        self.obj=ObjectRecord(0,0,model,(0.,0.,0.),(0,0,0),(1.,1.,1.),0,0)
    def __getattr__(self,name):return getattr(self.dec,name)
    def landtable(self,*args):return SimpleNamespace(texture_archive='OBJ_REGULAR')
    def collisions(self,table):return [self.row]
    def object(self,address):
        if address:raise ValueError('expected identity resource instance')
        return self.obj

def build(exe,system):
    d=PC2004StageDecoder(exe);pvm=PVMArchive(read_source(system,'OBJ_REGULAR.PVM'),'OBJ_REGULAR.PVM')
    if d.resolve_texture_archive(0x8B1CD8)!='OBJ_REGULAR':raise ValueError('wrong regular archive')
    pe=d.pe
    # Operand witnesses distinguish Basic model addresses from NJS_OBJECTs.
    for address,expected in [(0x7A49EC,bytes.fromhex('689c678b00')),
                             (0x7A4B3A,bytes.fromhex('68105c8b00'))]:
        if pe.read_bytes(address,len(expected))!=expected:raise ValueError('display model witness mismatch')
    cap=struct.unpack('<f',pe.read_bytes(0x7DCC74,4))[0]
    config=bytearray(32+6*32)
    struct.pack_into('<8sHHIIIII',config,0,b'SA3SPRV\0',1,0,6,32,32,0,len(config))
    chunks=[];report=[]
    for kind in range(2):
        for part in range(3):
            if part:
                obj=d.object(OBJECTS[kind][part-1]);model=obj.model_va;position=obj.position
            else:model=BASES[kind];position=(0.,0.,0.)
            i=kind*3+part
            struct.pack_into('<II3ffII',config,32+i*32,kind+1,part,*position,cap if part==2 else 0.,0,0)
            geometry,materials,meta=build_visual_chunk(Adapter(d,model),0,pvm.texture_count)
            materials,textures,used=compact_textures(materials,pvm)
            payload=sa3dspak.build([('METADATA',b'{"scope":"owner spring component"}',0),
                ('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)])
            chunks.append((f'VISUAL{i}',payload,0))
            report.append({'kind':kind+1,'part':part,'model':hex(model),'textures':used,'geometry':meta})
    metadata={'source_profile':'PC2004-spring-visual-v1','source_exe_sha256':d.sha256,
        'source_pvm_sha256':pvm.sha256(),'decoder_revision':'pc2004-pvm9-linear-v1','components':report,
        'scope':'original Basic geometry/material/texture; callback transforms remain runtime-owned'}
    data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('SPRING',config,0),*chunks])
    sa3dspak.parse(data)
    return data,metadata
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sonic_exe',type=Path)
    ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
    if args.output.resolve().is_relative_to(ROOT):raise SystemExit('owner output must remain outside source tree')
    data,report=build(args.sonic_exe,args.system);args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(data)
    print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'components':len(report['components'])}))
