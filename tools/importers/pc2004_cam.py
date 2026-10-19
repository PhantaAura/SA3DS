#!/usr/bin/env python3
"""Conservative PC2004 SADX CAM bank reader and pointer-free SA3DCAM v1.

Observed in owner 2004 system archives: 64-byte header, u32 LE count at +0,
60 zero reserved bytes; followed by count * 64-byte records.  First two
u16 and following u32 in each row are retained as *uninterpreted* words;
the remaining 56 bytes are kept bit-for-bit as opaque payload, not presumed
to be camera XYZ, FOV, triggers or an executable camera algorithm.

IMPORTANT: CAM0701S.BIN row 169 contains 0xffd00000 at record offset 32.
This decodes as NaN if viewed as float32, but is valid source data; do not
filter NaNs or normalize floating payloads. Do not infer behavior from fields.

Neutral SA3DCAM keeps all records verbatim, per-file identity/source SHA256,
CRC32, and contiguous offsets. It contains no PC pointers and no Sega data
is bundled with this module.
"""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
import binascii
import hashlib
from pathlib import Path
import re
import struct
from zipfile import ZipFile

SOURCE_HEADER_SIZE = 64
SOURCE_ROW_SIZE = 64
MAX_SOURCES = 1024
MAX_RECORDS = 250000
SOURCE_NAME = re.compile(r'^CAM[A-Z0-9_]+\.BIN$', re.ASCII)
NUMERIC_NAME = re.compile(r'^CAM([0-9]{2})([0-9]{2})([A-Z])\.BIN$', re.ASCII)
MAGIC = b'SA3DCAM\0'
VERSION = (1, 0)
HEADER = struct.Struct('<8sHHIIIIIII')
DIRECTORY = struct.Struct('<32sII32s')

class CamDecodeError(ValueError):
    pass

def align16(n: int) -> int:
    return (n + 15) & ~15

def camera_group(name: str) -> tuple[int,int,str] | None:
    m = NUMERIC_NAME.fullmatch(name.upper())
    return (int(m[1]), int(m[2]), m[3]) if m else None

@dataclass(frozen=True)
class CamRecord:
    raw: bytes
    word_00: int
    word_02: int
    word_04: int

@dataclass(frozen=True)
class CamSource:
    name: str
    sha256: str
    records: tuple[CamRecord, ...]

    def source_bytes(self) -> bytes:
        return struct.pack('<I', len(self.records)) + bytes(60) + b''.join(x.raw for x in self.records)

    def summary(self) -> dict:
        a = Counter(row.word_00 for row in self.records)
        b = Counter(row.word_02 for row in self.records)
        return {
            'name': self.name, 'sha256': self.sha256,
            'numeric_filename_group': camera_group(self.name),
            'record_count': len(self.records),
            'record_size': SOURCE_ROW_SIZE,
            'first_u16_histogram': {f'0x{x:04X}': a[x] for x in sorted(a)},
            'second_u16_histogram': {f'0x{x:04X}': b[x] for x in sorted(b)},
            'fields_semantics': 'unresolved; all 64 source bytes preserved',
        }

def parse_record(raw: bytes) -> CamRecord:
    if len(raw) != SOURCE_ROW_SIZE:
        raise CamDecodeError('CAM record must be 64 bytes')
    u16a, u16b, u32c = struct.unpack_from('<HHI', raw)
    return CamRecord(raw, u16a, u16b, u32c)

def read_cam_source(name: str, blob: bytes) -> CamSource:
    name = name.upper()
    if not SOURCE_NAME.fullmatch(name):
        raise CamDecodeError(f'not a recognized CAM filename: {name}')
    if len(name.encode('ascii')) >= 32:
        raise CamDecodeError('CAM filename too long for neutral directory')
    if len(blob) < SOURCE_HEADER_SIZE:
        raise CamDecodeError(f'{name}: truncated CAM header')
    count, = struct.unpack_from('<I', blob, 0)
    if count > MAX_RECORDS:
        raise CamDecodeError(f'{name}: excessive CAM row count')
    if blob[4:SOURCE_HEADER_SIZE] != bytes(60):
        raise CamDecodeError(f'{name}: unsupported nonzero CAM header fields')
    if len(blob) != SOURCE_HEADER_SIZE + count * SOURCE_ROW_SIZE:
        raise CamDecodeError(f'{name}: CAM record count/length mismatch')
    rows = tuple(parse_record(blob[SOURCE_HEADER_SIZE + i*SOURCE_ROW_SIZE:
                                  SOURCE_HEADER_SIZE + (i+1)*SOURCE_ROW_SIZE]) for i in range(count))
    result = CamSource(name, hashlib.sha256(blob).hexdigest(), rows)
    if result.source_bytes() != blob:
        raise CamDecodeError(f'{name}: CAM source roundtrip mismatch')
    return result

def list_cam_sources(source: Path, names: set[str] | None = None) -> list[CamSource]:
    source = Path(source)
    blobs: dict[str, bytes] = {}
    if source.is_dir():
        for p in source.rglob('*'):
            if not p.is_file(): continue
            name = p.name.upper()
            if not SOURCE_NAME.fullmatch(name) or (names is not None and name not in names):continue
            if name in blobs: raise CamDecodeError(f'ambiguous duplicate CAM source: {name}')
            blobs[name] = p.read_bytes()
    elif source.is_file() and source.suffix.lower() == '.zip':
        with ZipFile(source) as z:
            for item in z.infolist():
                if item.is_dir(): continue
                name = Path(item.filename).name.upper()
                if not SOURCE_NAME.fullmatch(name) or (names is not None and name not in names):continue
                if name in blobs: raise CamDecodeError(f'ambiguous duplicate CAM source: {name}')
                blobs[name] = z.read(item)
    else:
        raise CamDecodeError('CAM source must be existing system directory or ZIP')
    if names is not None and set(blobs) != names:
        raise CamDecodeError('missing CAM sources: ' + ', '.join(sorted(names - set(blobs))))
    if len(blobs) > MAX_SOURCES:
        raise CamDecodeError('too many CAM source files')
    return [read_cam_source(name, blobs[name]) for name in sorted(blobs)]

def build_cam_chunk(sources: list[CamSource]) -> bytes:
    if not (1 <= len(sources) <= MAX_SOURCES):
        raise CamDecodeError('CAM chunk must contain 1..1024 files')
    sources = sorted(sources, key=lambda x:x.name)
    if len({s.name for s in sources}) != len(sources):
        raise CamDecodeError('duplicate CAM source names')
    count = sum(len(s.records) for s in sources)
    if count > MAX_RECORDS:
        raise CamDecodeError('too many CAM records')
    dir_offset = align16(HEADER.size)
    rows_offset = align16(dir_offset + len(sources)*DIRECTORY.size)
    directory = bytearray()
    rows = bytearray()
    for src in sources:
        raw = src.source_bytes()
        if hashlib.sha256(raw).hexdigest() != src.sha256:
            raise CamDecodeError(f'{src.name}: CAM provenance mismatch')
        if not SOURCE_NAME.fullmatch(src.name) or len(src.name.encode('ascii')) >= 32:
            raise CamDecodeError(f'{src.name}: invalid CAM source name')
        directory += DIRECTORY.pack(src.name.encode('ascii').ljust(32,b'\0'),
                                    len(rows)//SOURCE_ROW_SIZE,len(src.records),bytes.fromhex(src.sha256))
        rows += raw[SOURCE_HEADER_SIZE:]
    size = rows_offset + len(rows)
    out=bytearray(size)
    HEADER.pack_into(out,0,MAGIC,*VERSION,HEADER.size,len(sources),count,
                     dir_offset,rows_offset,size,binascii.crc32(rows)&0xffffffff)
    out[dir_offset:dir_offset+len(directory)]=directory
    out[rows_offset:]=rows
    return bytes(out)

def parse_cam_chunk(blob: bytes) -> dict:
    if len(blob)<HEADER.size:raise CamDecodeError('truncated neutral CAM header')
    magic,major,minor,header_size,nfiles,count,dir_offset,rows_offset,size,crc=HEADER.unpack_from(blob)
    if magic!=MAGIC or (major,minor)!=VERSION or header_size!=HEADER.size:
        raise CamDecodeError('bad neutral CAM header/version')
    if not 1<=nfiles<=MAX_SOURCES or count>MAX_RECORDS:
        raise CamDecodeError('neutral CAM record/source count invalid')
    if dir_offset!=align16(HEADER.size) or rows_offset!=align16(dir_offset+nfiles*DIRECTORY.size):
        raise CamDecodeError('neutral CAM offset invalid')
    if size!=len(blob) or rows_offset+count*SOURCE_ROW_SIZE!=size:
        raise CamDecodeError('neutral CAM size inconsistent')
    if any(blob[HEADER.size:dir_offset]) or any(blob[dir_offset+nfiles*DIRECTORY.size:rows_offset]):
        raise CamDecodeError('nonzero padding in neutral CAM')
    data=blob[rows_offset:]
    if binascii.crc32(data)&0xffffffff!=crc:
        raise CamDecodeError('CAM rows CRC mismatch')
    seen=set();expected_first=0;items=[]
    for i in range(nfiles):
        raw_name,first,n,digest=DIRECTORY.unpack_from(blob,dir_offset+i*DIRECTORY.size)
        if b'\0' not in raw_name:
            raise CamDecodeError('CAM name not terminated')
        name=raw_name.split(b'\0',1)[0].decode('ascii','strict')
        if raw_name!=name.encode().ljust(32,b'\0'):
            raise CamDecodeError('noncanonical CAM name padding')
        if not SOURCE_NAME.fullmatch(name) or name in seen or first!=expected_first or n>count-first:
            raise CamDecodeError('invalid/duplicate CAM directory range')
        seen.add(name);expected_first+=n
        rows=data[first*SOURCE_ROW_SIZE:(first+n)*SOURCE_ROW_SIZE]
        raw=struct.pack('<I',n)+bytes(60)+rows
        if hashlib.sha256(raw).digest()!=digest:
            raise CamDecodeError(f'{name}: CAM SHA256 mismatch')
        items.append({'name':name,'record_count':n,'first_record':first,
                      'sha256':digest.hex(),'numeric_filename_group':camera_group(name)})
    if expected_first!=count:
        raise CamDecodeError('unaccounted CAM records')
    return {'files':nfiles,'records':count,'bytes':size,'sources':items,
            'semantics':'raw placement/setup records only; camera modes/triggers/behavior unverified'}
