#!/usr/bin/env python3
"""SA3C v0 pointer-free character container.

This module defines the code-only container writer/reader.  Generated packages
may contain source-derived game assets and therefore are deliberately excluded
from SA3DS code checkpoint archives.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import hashlib
import struct

MAGIC=b'SA3C'
VERSION_MAJOR=0
VERSION_MINOR=0
HEADER_SIZE=272
SECTION_ORDER=(b'NODE',b'MODL',b'MESH',b'VRTX',b'INDX',b'MATL',b'TEXS',b'ANIM',b'CHAN',b'KEYS')
SECTION_ENTRY_SIZE=20
INVALID_U16=0xFFFF

STRIDES={
    b'NODE':48,
    b'MODL':32,
    b'MESH':24,
    b'VRTX':32,
    b'INDX':2,
    b'MATL':32,
    b'TEXS':24,
    b'ANIM':24,
    b'CHAN':24,
    b'KEYS':16,
}

class SA3CError(ValueError):
    pass

@dataclass(frozen=True)
class SectionInfo:
    tag: bytes
    count: int
    offset: int
    stride: int
    size: int


def _align(value:int, alignment:int=16)->int:
    return (value + alignment - 1) & ~(alignment - 1)


def build(sections:dict[bytes,bytes], counts:dict[bytes,int], source_sha256_hex:str, flags:int=0)->bytes:
    if set(sections) != set(SECTION_ORDER) or set(counts) != set(SECTION_ORDER):
        raise SA3CError('sections/counts must exactly match SA3C v0 section set')
    source_hash=bytes.fromhex(source_sha256_hex)
    if len(source_hash)!=32:
        raise SA3CError('source hash must be SHA-256')
    infos=[]
    cursor=HEADER_SIZE
    for tag in SECTION_ORDER:
        stride=STRIDES[tag]
        data=sections[tag]
        count=counts[tag]
        if len(data)!=count*stride:
            raise SA3CError(f'{tag!r}: size {len(data)} != count*stride {count*stride}')
        cursor=_align(cursor)
        infos.append(SectionInfo(tag,count,cursor,stride,len(data)))
        cursor += len(data)
    file_size=_align(cursor)
    out=bytearray(file_size)
    struct.pack_into('<4sHHIIII32s',out,0,MAGIC,VERSION_MAJOR,VERSION_MINOR,HEADER_SIZE,len(SECTION_ORDER),flags,file_size,source_hash)
    dir_off=64
    for i,info in enumerate(infos):
        struct.pack_into('<4sIIII',out,dir_off+i*SECTION_ENTRY_SIZE,info.tag,info.count,info.offset,info.stride,info.size)
        out[info.offset:info.offset+info.size]=sections[info.tag]
    return bytes(out)


def parse(data:bytes)->dict:
    if 64 + len(SECTION_ORDER)*SECTION_ENTRY_SIZE > HEADER_SIZE:
        raise SA3CError('internal SA3C header directory exceeds header')
    if len(data)<HEADER_SIZE:
        raise SA3CError('truncated header')
    magic,major,minor,header_size,nsec,flags,file_size,source_hash=struct.unpack_from('<4sHHIIII32s',data,0)
    if magic!=MAGIC or (major,minor)!=(VERSION_MAJOR,VERSION_MINOR):
        raise SA3CError('unsupported SA3C version')
    if header_size!=HEADER_SIZE or nsec!=len(SECTION_ORDER) or file_size!=len(data):
        raise SA3CError('header size/count/file length mismatch')
    infos={}
    ranges=[]
    dir_off=64
    for i in range(nsec):
        tag,count,offset,stride,size=struct.unpack_from('<4sIIII',data,dir_off+i*SECTION_ENTRY_SIZE)
        if tag not in STRIDES or stride!=STRIDES[tag] or size!=count*stride:
            raise SA3CError('invalid section directory entry')
        if offset<HEADER_SIZE or offset+size>len(data):
            raise SA3CError('section outside file')
        infos[tag]=SectionInfo(tag,count,offset,stride,size)
        if size:
            ranges.append((offset,offset+size,tag))
    if set(infos)!=set(SECTION_ORDER):
        raise SA3CError('missing/duplicate section tags')
    ranges.sort()
    for a,b in zip(ranges,ranges[1:]):
        if a[1]>b[0]:
            raise SA3CError('overlapping sections')
    return {
        'version':(major,minor),
        'flags':flags,
        'file_size':file_size,
        'source_sha256':source_hash.hex(),
        'sections':infos,
    }


def inspect_file(path:Path)->dict:
    return parse(Path(path).read_bytes())
