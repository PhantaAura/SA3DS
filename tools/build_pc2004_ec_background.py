#!/usr/bin/env python3
"""Owner-only original PC2004 Emerald Coast sky/horizon package conversion.

Uses the original two background Basic-object roots and BG_BEACH texture list,
not generated sky geometry. Output contains Sega-derived assets and must remain
outside the source checkout. Runtime projection/scale rows are read from the
exact executable, not embedded game assets in project source.
"""
from pathlib import Path
from types import SimpleNamespace
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.importers.pc2004_stage import PC2004StageDecoder,CollisionRecord
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,build_material_chunk,build_sa3t

ROOTS=(0x103B270,0x103B37C)
TEXTURE_LIST=0x103B3D4
TABLE_SCALE=0xE99E6C;TABLE_BACKGROUND_CLIP=0xE99E78
TABLE_TERRAIN_CLIP=0xE99E84;TABLE_FOG=0xE99E90
CONFIG_HEADER=struct.Struct('<8sHHIIIII')
CONFIG_ROW=struct.Struct('<II3f4f2fIIIII')
assert CONFIG_HEADER.size==32 and CONFIG_ROW.size==64

def config_blob(rows):
    if len(rows)!=6:raise ValueError('two acts x three clip levels required')
    out=bytearray(32+len(rows)*64)
    CONFIG_HEADER.pack_into(out,0,b'SA3DBG\0\0',1,0,6,64,32,3,0)
    for i,row in enumerate(rows):CONFIG_ROW.pack_into(out,32+i*64,*row)
    return bytes(out)

def source_rows(dec):
    pe=dec.pe;rows=[]
    for act in range(2):
        scale=pe.read_u32(TABLE_SCALE+act*4)
        bgclip=pe.read_u32(TABLE_BACKGROUND_CLIP+act*4)
        terrain=pe.read_u32(TABLE_TERRAIN_CLIP+act*4)
        fog=pe.read_u32(TABLE_FOG+act*4)
        for clip in range(3):
            xyz=struct.unpack('<3f',pe.read_bytes(scale+clip*12,12))
            bn,bf=struct.unpack('<2f',pe.read_bytes(bgclip+clip*8,8))
            tn,tf=struct.unpack('<2f',pe.read_bytes(terrain+clip*8,8))
            fn,ff,fc,fe=struct.unpack('<ffII',pe.read_bytes(fog+clip*16,16))
            rows.append((act,clip,*xyz,-bn,-(bf-1000.0),-tn,-tf,-fn,-ff,fc,fe,0x003946,3,0))
    return rows

def make_package(visual,rows,metadata):
    return sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),
        ('BACKGROUND',config_blob(rows),0),('VISUAL',visual,0)])

def build(exe,pvm_path):
    dec=PC2004StageDecoder(exe)
    if dec.resolve_texture_archive(TEXTURE_LIST)!='BG_BEACH':raise ValueError('background texture-list identity changed')
    pvm=PVMArchive(pvm_path);rows=[]
    for index,va in enumerate(ROOTS):
        obj=dec.object(va);model=dec.model(obj.model_va)
        if obj.flags!=0x17 or obj.child_va or obj.sibling_va or obj.position!=(0.,0.,0.) or obj.rotation_bams!=(0,0,0) or obj.scale!=(1.,1.,1.):
            raise ValueError('unsupported original background hierarchy/transform')
        # Renderer-only format adapter: these are not original COL records and
        # never feed collision. Visible classification lets the existing native
        # basic-model batching path consume original background meshes.
        rows.append(CollisionRecord(index,model.center,model.radius,0,0,va,0,0x80000000))
    original_landtable=dec.landtable;original_collisions=dec.collisions
    dec.landtable=lambda level,act:SimpleNamespace(texture_archive='BG_BEACH')
    dec.collisions=lambda table:rows
    geometry,materials,visual_meta=build_visual_chunk(dec,0,pvm.texture_count)
    dec.landtable=original_landtable;dec.collisions=original_collisions
    textures=build_sa3t(pvm)
    metadata={'source_profile':'SADX-PC2004-EC-background-v1','source_exe_sha256':dec.sha256,
        'texture_archive':'BG_BEACH','texture_source_sha256':pvm.sha256(),
        'source_roots':[hex(x) for x in ROOTS],'geometry':visual_meta,
        'camera_anchor':'cameraX,0,cameraZ','fog_during_background':False,
        'collision':False,'source_geometry_preserved':True}
    visual=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),
        ('GEOMETRY',geometry,0),('MATERIALS',build_material_chunk(materials),0),('TEXTURES',textures,0)])
    result=make_package(visual,source_rows(dec),metadata)
    report={**metadata,'file_bytes':len(result),'sha256':hashlib.sha256(result).hexdigest(),
        'config_rows':source_rows(dec),'native_integration':'PENDING'}
    return result,report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True);p.add_argument('--pvm',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path)
    a=p.parse_args()
    if a.output.resolve().is_relative_to(ROOT) or (a.report and a.report.resolve().is_relative_to(ROOT)):
        p.error('owner-generated background assets/reports must stay outside source checkout')
    output,report=build(a.exe,a.pvm);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(output)
    if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('file_bytes','sha256','texture_archive','geometry')},indent=2))
if __name__=='__main__':main()
