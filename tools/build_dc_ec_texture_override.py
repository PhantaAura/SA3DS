#!/usr/bin/env python3
"""Convert one local Dreamcast EC texture to a bounded native SD override.

Output contains owner-derived pixels and must remain outside the code checkout.
No automatic download, game install mutation, or DLL execution is performed.
"""
from pathlib import Path
import argparse,hashlib,json,struct,sys,zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools import sa3t,sa3dspak
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.dreamcast_pvm import DreamcastPVM,decompress_prs

MAX_BYTES=64+96+256*256*4

def build_override(texture_blob:bytes,baseline_sha256:str,act:int,target_index:int)->bytes:
    archive=sa3t.parse(texture_blob)
    if len(archive['textures'])!=1 or act not in (0,1) or not 0<=target_index<4096:
        raise ValueError('one replacement and valid Sonic act/index required')
    d=archive['textures'][0]
    if any(n<8 or n>256 or n&(n-1) for n in (d.width,d.height)):
        raise ValueError('replacement dimensions must be8..256 powers-of-two')
    baseline=bytes.fromhex(baseline_sha256)
    if len(baseline)!=32:raise ValueError('baseline SHA256 required')
    out=bytearray(64+len(texture_blob))
    struct.pack_into('<8sHHIIIII32s',out,0,b'SA3DMOD\0',1,0,64,len(out),act,target_index,0,baseline)
    out[64:]=texture_blob
    struct.pack_into('<I',out,28,zlib.crc32(out))
    if len(out)>MAX_BYTES:raise ValueError('override exceeds runtime cap')
    return bytes(out)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',required=True,type=Path,help='local Dreamcast PVM or PVM.PRS')
    p.add_argument('--pc-pvm',required=True,type=Path,help='original PC2004 act PVM for exact target name/index')
    p.add_argument('--baseline-package',required=True,type=Path,help='owner-generated native EC act package')
    p.add_argument('--act',required=True,type=int,choices=(0,1))
    p.add_argument('--name',default='m128_cos002')
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--report',type=Path)
    a=p.parse_args()
    if a.output.resolve().is_relative_to(ROOT) or (a.report and a.report.resolve().is_relative_to(ROOT)):
        p.error('owner pixels/reports must be generated outside the code checkout')
    source=a.source.read_bytes();decoded=source if source[:4]==b'PVMH' else decompress_prs(source)
    dc=DreamcastPVM(decoded);entry=dc.named(a.name);rgba=dc.decode_base_rgba(entry.index)
    pc=PVMArchive(a.pc_pvm)
    matches=[e for e in pc.entries if e.name.casefold()==a.name.casefold()]
    if len(matches)!=1:raise ValueError('PC target name missing/ambiguous')
    target=matches[0]
    if (entry.width,entry.height)!=(target.width,target.height):raise ValueError('PC/DC target dimensions differ')
    parsed=sa3dspak.parse(a.baseline_package.read_bytes())
    chunks=[c for c in parsed['chunks'] if c['type']=='TEXTURES']
    if len(chunks)!=1:raise ValueError('baseline textures missing/ambiguous')
    package=a.baseline_package.read_bytes();c=chunks[0]
    baseline=sa3t.parse(package[c['offset']:c['offset']+c['size']])
    if baseline['source_sha256']!=pc.sha256():raise ValueError('baseline texture archive differs from PC PVM')
    blob=sa3t.build([dict(width=entry.width,height=entry.height,rgba=rgba,
        source_pixel_format=entry.pixel_format,source_data_format=entry.data_format,global_index=entry.global_index)],dc.sha256())
    result=build_override(blob,pc.sha256(),a.act,target.index)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(result)
    original=pc.decode_base_rgba(target.index)
    report=dict(act=a.act,name=a.name,target_texture=target.index,source_texture=entry.index,
        width=entry.width,height=entry.height,source_compressed_sha256=hashlib.sha256(source).hexdigest(),
        source_pvm_sha256=dc.sha256(),baseline_pvm_sha256=pc.sha256(),
        converted_rgba_sha256=hashlib.sha256(rgba).hexdigest(),original_rgba_sha256=hashlib.sha256(original).hexdigest(),
        differing_rgba_components=sum(x!=y for x,y in zip(rgba,original)),
        override_bytes=len(result),override_sha256=hashlib.sha256(result).hexdigest())
    if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
