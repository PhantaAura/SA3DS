#!/usr/bin/env python3
"""Evidence report for Emerald Coast visual/material/PVM decoding."""
from __future__ import annotations
from pathlib import Path
from collections import Counter
import argparse, json, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder, EMERALD_COAST_LEVEL_ID
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_emerald_coast_visual import read_source, build_visual_chunk, parse_visual_chunk

PVM_NAMES=("BEACH01.PVM","BEACH02.PVM","BEACH03.PVM","OBJ_BEACH.PVM","BEACH_SEA.PVM","BG_BEACH.PVM")

def pvm_summary(p:PVMArchive):
    dims=Counter(f"{e.width}x{e.height}" for e in p.entries)
    pf=Counter(str(e.pixel_format) for e in p.entries)
    df=Counter(str(e.data_format) for e in p.entries)
    return {
        "name":p.label,"size":len(p.data),"sha256":p.sha256(),"flags":f"0x{p.flags:04x}",
        "texture_count":p.texture_count,"pixel_formats":dict(sorted(pf.items())),
        "data_formats":dict(sorted(df.items())),"dimensions":dict(sorted(dims.items())),
        "neutral_rgba8_bytes":sum(e.width*e.height*4 for e in p.entries),
    }

def main(argv=None):
    ap=argparse.ArgumentParser();ap.add_argument('exe',type=Path);ap.add_argument('system_source',type=Path);ap.add_argument('--json',type=Path)
    a=ap.parse_args(argv)
    dec=PC2004StageDecoder(a.exe)
    pvms={}
    for name in PVM_NAMES:
        pvms[name]=PVMArchive(read_source(a.system_source,name),name)
    acts=[]
    for act in (0,1,2):
        table=dec.landtable(EMERALD_COAST_LEVEL_ID,act)
        p=pvms[table.texture_archive+'.PVM']
        vis,mats,meta=build_visual_chunk(dec,act,p.texture_count)
        acts.append({
            "act":act,"landtable_va":f"0x{table.va:08x}","texture_archive":table.texture_archive,
            "texture_count":p.texture_count,"visual":parse_visual_chunk(vis),
            "materials_deduplicated":len(mats),"uv":meta["uv"],
            "meshes_without_uv":meta["counts"]["meshes_without_uv"],
        })
    # Strong base-first witness: identical named base image appears in a
    # base-only 0x010f archive and a full-mip 0x000f archive.
    b1=pvms['BEACH01.PVM'];b3=pvms['BEACH03.PVM']
    m1={e.name:e for e in b1.entries};m3={e.name:e for e in b3.entries}
    witness='m128_cos002'
    e1,e3=m1[witness],m3[witness]
    witness_equal=b1.decode_base_rgba(e1.index)==b3.decode_base_rgba(e3.index)
    comparable=equal=0
    for name in set(m1)&set(m3):
        x,y=m1[name],m3[name]
        if (x.width,x.height,x.pixel_format)==(y.width,y.height,y.pixel_format):
            comparable+=1
            if b1.decode_base_rgba(x.index)==b3.decode_base_rgba(y.index):equal+=1
    report={
        "schema":"sa3ds-emerald-coast-visual-recon-v1",
        "version":"0.0.9.1B",
        "source_profile":"SADX_PC_2004_CANONICAL",
        "exe_sha256":dec.sha256,
        "pvm_archives":[pvm_summary(pvms[n]) for n in PVM_NAMES],
        "acts":acts,
        "format9_base_order_evidence":{
            "largest_mip":"first",
            "witness_texture":witness,
            "base_only_archive":"BEACH01.PVM",
            "mipped_archive":"BEACH03.PVM",
            "decoded_rgba_equal":witness_equal,
            "same_name_comparable_textures":comparable,
            "same_name_equal_base_images":equal,
        },
        "limits":{
            "material_flags":"preserved raw; bit semantics not assigned",
            "sa3t_rgba8":"neutral validation/import representation; final Old-3DS texture residency format pending",
            "set_cam_objects":"not part of 0.0.9.1B",
        },
    }
    if not witness_equal: raise SystemExit('base-first cross-archive witness failed')
    text=json.dumps(report,indent=2,sort_keys=True)
    if a.json:a.json.write_text(text+'\n')
    print(text)
    return 0
if __name__=='__main__':raise SystemExit(main())
