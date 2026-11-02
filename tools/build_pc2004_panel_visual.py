#!/usr/bin/env python3
"""Convert the owner's exact dash-panel ACTION to external component assets."""
from pathlib import Path
import argparse,hashlib,json,math,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_chrmodels import SonicDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_spring_visual import Adapter
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,read_source
from tools.build_pc2004_ring_assets import compact_textures
from tools.verify_pc2004_dash_panel import verify

def build(exe,system):
    verify(exe);d=PC2004StageDecoder(exe)
    decoder=SonicDecoder.__new__(SonicDecoder);decoder.pe=d.pe
    nodes=decoder.hierarchy(0x8b8b34);motion=decoder.motion(0x8b8bb8,len(nodes))
    if len(nodes)!=3 or motion.frame_count!=8 or motion.type_bits!=3:raise ValueError('panel ACTION profile')
    lookup={n.va:i for i,n in enumerate(nodes)};parents={nodes[0].va:65535}
    def parent_walk(va,parent):
        while va:
            n=d.object(va);parents[va]=parent
            if n.child_va:parent_walk(n.child_va,lookup[va])
            va=n.sibling_va
    parent_walk(nodes[0].va,65535)
    poseoff=32+len(nodes)*16;total=poseoff+8*len(nodes)*36
    config=bytearray(total);struct.pack_into('<8s6I',config,0,b'SA3PANL\0',1,3,8,32,poseoff,total)
    pvm=PVMArchive(read_source(system,'OBJ_REGULAR.PVM'),'OBJ_REGULAR.PVM');chunks=[];report=[]
    for i,n in enumerate(nodes):
        if not n.model_va or n.flags&0x40:raise ValueError('unsupported model/skip node')
        struct.pack_into('<HHIII',config,32+i*16,parents[n.va],i,n.flags,0,0)
        pos,ang,npos,nang=struct.unpack('<4I',d.pe.read_bytes(motion.data_va+i*16,16))
        if npos or pos:raise ValueError('unexpected translation channel')
        keys=list(struct.iter_unpack('<Iiii',d.pe.read_bytes(ang,nang*16))) if nang else []
        if keys and ([k[0] for k in keys]!=[0,7] or i!=1):raise ValueError('unexpected panel angle keys')
        for frame in range(8):
            angles=n.rotation_bams
            if keys:
                t=frame/7.;angles=tuple(math.trunc(keys[0][axis]*(1.-t)+keys[1][axis]*t) for axis in (1,2,3))
            struct.pack_into('<3f3i3f',config,poseoff+(frame*3+i)*36,*n.position,*angles,*n.scale)
        geometry,materials,meta=build_visual_chunk(Adapter(d,n.model_va),0,pvm.texture_count)
        materials,textures,used=compact_textures(materials,pvm)
        payload=sa3dspak.build([('METADATA',b'{"scope":"owner panel component"}',0),
            ('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)])
        chunks.append((f'VISUAL{i}',payload,0));report.append({'node':i,'parent':parents[n.va],
            'model':hex(n.model_va),'textures':used,'geometry':meta,'flags':n.flags})
    metadata={'source_profile':'PC2004-panel-visual-v1','source_exe_sha256':d.sha256,
        'source_pvm_sha256':pvm.sha256(),'decoder_revision':'pc2004-pvm9-linear-v1',
        'nodes':report,'frames':8,'sampling':'raw int32 BAMS linear then truncate; integer display phases'}
    data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('PANEL',config,0),*chunks])
    sa3dspak.parse(data);return data,metadata
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sonic_exe',type=Path)
    ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
    if args.output.resolve().is_relative_to(ROOT):raise SystemExit('owner output must remain outside source tree')
    data,report=build(args.sonic_exe,args.system);args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(data);print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'nodes':len(report['nodes']),'frames':8}))
