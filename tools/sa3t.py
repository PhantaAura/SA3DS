#!/usr/bin/env python3
"""SA3T v0 pointer-free texture package.

Generated packages contain source-derived texture pixels and are intentionally
excluded from code-only SA3DS checkpoints.  The format keeps texture residency
separate from SA3C character geometry/animation.
"""
from __future__ import annotations
from dataclasses import dataclass
import hashlib, struct

MAGIC=b'SA3T'; VERSION_MAJOR=0; VERSION_MINOR=0
HEADER_SIZE=64; DESC_SIZE=24; FORMAT_RGBA8=1

@dataclass(frozen=True)
class TextureDesc:
    width:int; height:int; format:int; flags:int
    data_offset:int; data_size:int
    source_pixel_format:int; source_data_format:int; global_index:int

def _align(v:int,a:int=16)->int:return (v+a-1)&~(a-1)

def build(entries:list[dict],source_sha256_hex:str)->bytes:
    source=bytes.fromhex(source_sha256_hex)
    if len(source)!=32:raise ValueError('source hash must be SHA-256')
    count=len(entries); directory_end=HEADER_SIZE+count*DESC_SIZE
    cursor=_align(directory_end,16); desc=[]
    for e in entries:
        pixels=bytes(e['rgba'])
        expected=int(e['width'])*int(e['height'])*4
        if len(pixels)!=expected:raise ValueError('RGBA8 payload size mismatch')
        cursor=_align(cursor,16)
        desc.append(TextureDesc(int(e['width']),int(e['height']),FORMAT_RGBA8,0,cursor,len(pixels),int(e['source_pixel_format']),int(e['source_data_format']),int(e['global_index'])))
        cursor+=len(pixels)
    size=_align(cursor,16);out=bytearray(size)
    struct.pack_into('<4sHHIII32s12x',out,0,MAGIC,VERSION_MAJOR,VERSION_MINOR,HEADER_SIZE,count,size,source)
    for i,(d,e) in enumerate(zip(desc,entries)):
        struct.pack_into('<HHHHIIHHI',out,HEADER_SIZE+i*DESC_SIZE,d.width,d.height,d.format,d.flags,d.data_offset,d.data_size,d.source_pixel_format,d.source_data_format,d.global_index)
        out[d.data_offset:d.data_offset+d.data_size]=bytes(e['rgba'])
    return bytes(out)

def parse(data:bytes)->dict:
    if len(data)<HEADER_SIZE:raise ValueError('truncated SA3T')
    magic,maj,min_,hs,count,size,source=struct.unpack_from('<4sHHIII32s',data,0)
    if magic!=MAGIC or (maj,min_)!=(0,0) or hs!=HEADER_SIZE or size!=len(data):raise ValueError('bad SA3T header')
    if HEADER_SIZE+count*DESC_SIZE>len(data):raise ValueError('truncated SA3T directory')
    entries=[];last=HEADER_SIZE+count*DESC_SIZE
    for i in range(count):
        v=struct.unpack_from('<HHHHIIHHI',data,HEADER_SIZE+i*DESC_SIZE)
        d=TextureDesc(*v)
        if d.format!=FORMAT_RGBA8 or d.data_size!=d.width*d.height*4:raise ValueError('unsupported SA3T texture')
        if d.data_offset%16 or d.data_offset<last or d.data_offset+d.data_size>len(data):raise ValueError('bad SA3T range')
        last=d.data_offset+d.data_size;entries.append(d)
    return {'version':(maj,min_),'source_sha256':source.hex(),'textures':entries,'file_size':size}
