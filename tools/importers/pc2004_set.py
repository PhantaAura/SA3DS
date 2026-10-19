#!/usr/bin/env python3
"""Evidence-conservative PC SADX SET reader and pointer-free SA3DS SET chunk.

On-disk SET observation: 32-byte header (u32 count + 28 reserved zero
bytes), followed by count 32-byte rows.  Each row contains a u16 type word,
three signed 16-bit angle words, XYZ float32 position, and three float32
object-specific parameters.  The *meaning* of type-word high bits, angles,
and parameters is not proven by the file layout alone.  Do not interpret the
last three floats as an object scale or infer gameplay object classes.

The SA3DSET chunk stores original rows bit-for-bit, with a pointer-free file
directory (ASCII names, ranges, source SHA-256).  It contains no original PC
pointers or address assumptions.  This is decoding/placement data, NOT an
implemented gameplay object registry/spawner.
"""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
import binascii
import hashlib
import math
from pathlib import Path
import re
import struct
from zipfile import ZipFile

SOURCE_HEADER_SIZE = 32
SOURCE_ROW = struct.Struct('<Hhhhffffff')
SOURCE_RECORD_SIZE = SOURCE_ROW.size
assert SOURCE_RECORD_SIZE == 32
MAX_RECORDS = 250_000
MAX_FILES = 1_024
SOURCE_NAME = re.compile(r'^SET([0-9]{2})([0-9]{2})([A-Z])\.BIN$', re.ASCII)

MAGIC = b'SA3DSET\0'
VERSION = (1, 0)
HEADER = struct.Struct('<8sHHIIIIIII')
DIRECTORY = struct.Struct('<16sII32s')

class SetDecodeError(ValueError):
    pass


def align16(n: int) -> int:
    return (n + 15) & ~15


def source_name_info(name: str) -> tuple[int, int, str]:
    match = SOURCE_NAME.fullmatch(name.upper())
    if not match:
        raise SetDecodeError(f'not a standard SETnnnnX.BIN filename: {name}')
    return int(match[1]), int(match[2]), match[3]


@dataclass(frozen=True)
class Placement:
    type_word: int
    angle_words: tuple[int, int, int]
    position: tuple[float, float, float]
    parameters: tuple[float, float, float]
    raw: bytes


def parse_placement(raw: bytes) -> Placement:
    if len(raw) != SOURCE_RECORD_SIZE:
        raise SetDecodeError('SET placement record is not 32 bytes')
    values = SOURCE_ROW.unpack(raw)
    if not all(math.isfinite(v) for v in values[4:]):
        raise SetDecodeError('non-finite position/parameter field')
    return Placement(values[0], tuple(values[1:4]), tuple(values[4:7]), tuple(values[7:10]), raw)


@dataclass(frozen=True)
class SetSource:
    name: str
    level_id: int
    act: int
    variant: str
    sha256: str
    records: tuple[Placement, ...]

    def source_bytes(self) -> bytes:
        header = struct.pack('<I', len(self.records)) + bytes(28)
        return header + b''.join(p.raw for p in self.records)

    def summary(self) -> dict:
        types = Counter(p.type_word for p in self.records)
        return {
            'name': self.name,
            'level_id_from_filename': self.level_id,
            'act_from_filename': self.act,
            'variant_from_filename': self.variant,
            'sha256': self.sha256,
            'record_count': len(self.records),
            'unique_type_words': len(types),
            'type_word_histogram': {f'0x{k:04X}': types[k] for k in sorted(types)},
            'high_bits_are_unresolved': True,
            'parameters_are_object_specific_not_assumed_scale': True,
        }


def read_set_source(name: str, blob: bytes) -> SetSource:
    name = name.upper()
    level, act, variant = source_name_info(name)
    if len(blob) < SOURCE_HEADER_SIZE:
        raise SetDecodeError(f'{name}: truncated header')
    count, = struct.unpack_from('<I', blob)
    if count > MAX_RECORDS:
        raise SetDecodeError(f'{name}: unreasonable row count {count}')
    if blob[4:SOURCE_HEADER_SIZE] != bytes(SOURCE_HEADER_SIZE - 4):
        raise SetDecodeError(f'{name}: unexpected nonzero header bytes, unsupported SET variant')
    if len(blob) != SOURCE_HEADER_SIZE + count * SOURCE_RECORD_SIZE:
        raise SetDecodeError(f'{name}: count/length mismatch ({count} records, {len(blob)} bytes)')
    rows = tuple(parse_placement(blob[SOURCE_HEADER_SIZE + n * SOURCE_RECORD_SIZE:
                                      SOURCE_HEADER_SIZE + (n + 1) * SOURCE_RECORD_SIZE]) for n in range(count))
    result = SetSource(name, level, act, variant, hashlib.sha256(blob).hexdigest(), rows)
    if result.source_bytes() != blob:
        raise SetDecodeError(f'{name}: lossless SET round-trip failed')
    return result


def list_set_sources(source: Path, names: set[str] | None = None) -> list[SetSource]:
    """Collect unambiguous SET files from a local owner folder or ZIP.

    Duplicate basenames are an error, not silently picked using ZIP ordering.
    """
    source = Path(source)
    candidates: dict[str, bytes] = {}
    if source.is_dir():
        paths = (p for p in source.rglob('*') if p.is_file())
        for p in paths:
            n = p.name.upper()
            if not SOURCE_NAME.fullmatch(n) or (names is not None and n not in names):
                continue
            if n in candidates:
                raise SetDecodeError(f'ambiguous duplicate SET filename: {n}')
            candidates[n] = p.read_bytes()
    elif source.is_file() and source.suffix.lower() == '.zip':
        with ZipFile(source) as archive:
            for info in archive.infolist():
                if info.is_dir():
                    continue
                n = Path(info.filename).name.upper()
                if not SOURCE_NAME.fullmatch(n) or (names is not None and n not in names):
                    continue
                if n in candidates:
                    raise SetDecodeError(f'ambiguous duplicate SET filename: {n}')
                candidates[n] = archive.read(info)
    else:
        raise SetDecodeError('source must be an existing system directory or ZIP')
    if names is not None and set(candidates) != names:
        raise SetDecodeError('missing SET files: ' + ', '.join(sorted(names - set(candidates))))
    if len(candidates) > MAX_FILES:
        raise SetDecodeError('too many SET source files')
    return [read_set_source(name, candidates[name]) for name in sorted(candidates)]


def build_set_chunk(sources: list[SetSource]) -> bytes:
    if not sources or len(sources) > MAX_FILES:
        raise SetDecodeError('SET chunk must have 1..1024 source files')
    sources = sorted(sources, key=lambda x: x.name)
    if len({s.name for s in sources}) != len(sources):
        raise SetDecodeError('duplicate SET source names')
    total_rows = sum(len(s.records) for s in sources)
    if total_rows > MAX_RECORDS:
        raise SetDecodeError('too many SET placements for a single chunk')
    directory_offset = align16(HEADER.size)
    rows_offset = align16(directory_offset + len(sources) * DIRECTORY.size)
    records = bytearray()
    directory = bytearray()
    for src in sources:
        source_name_info(src.name)
        source_bytes = src.source_bytes()
        if hashlib.sha256(source_bytes).hexdigest() != src.sha256:
            raise SetDecodeError(f'{src.name}: source changed since decoding')
        if len(src.name.encode('ascii')) > 16:
            raise SetDecodeError('SET source name exceeds directory width')
        directory += DIRECTORY.pack(src.name.encode('ascii').ljust(16, b'\0'),
                                    len(records) // SOURCE_RECORD_SIZE, len(src.records),
                                    bytes.fromhex(src.sha256))
        records += source_bytes[SOURCE_HEADER_SIZE:]
    total_size = rows_offset + len(records)
    blob = bytearray(total_size)
    HEADER.pack_into(blob, 0, MAGIC, *VERSION, HEADER.size, len(sources), total_rows,
                     directory_offset, rows_offset, total_size, binascii.crc32(records) & 0xFFFFFFFF)
    blob[directory_offset:directory_offset + len(directory)] = directory
    blob[rows_offset:] = records
    return bytes(blob)


def parse_set_chunk(blob: bytes) -> dict:
    if len(blob) < HEADER.size:
        raise SetDecodeError('truncated SA3DSET header')
    magic, major, minor, header_size, num_files, count, directory_offset, rows_offset, total_size, crc = HEADER.unpack_from(blob)
    if magic != MAGIC or (major, minor) != VERSION or header_size != HEADER.size:
        raise SetDecodeError('bad SA3DSET magic/version/header size')
    if not (1 <= num_files <= MAX_FILES and count <= MAX_RECORDS):
        raise SetDecodeError('bad SA3DSET source/row count')
    if directory_offset != align16(HEADER.size):
        raise SetDecodeError('SA3DSET directory misaligned or unsupported')
    if rows_offset != align16(directory_offset + num_files * DIRECTORY.size):
        raise SetDecodeError('SA3DSET record offset invalid')
    if total_size != len(blob) or rows_offset + count * SOURCE_RECORD_SIZE != len(blob):
        raise SetDecodeError('SA3DSET size mismatch')
    if any(blob[HEADER.size:directory_offset]) or any(blob[directory_offset + num_files * DIRECTORY.size:rows_offset]):
        raise SetDecodeError('SA3DSET padding must be zero')
    rows_blob = blob[rows_offset:]
    if binascii.crc32(rows_blob) & 0xFFFFFFFF != crc:
        raise SetDecodeError('SA3DSET records CRC mismatch')
    seen = set()
    result_sources = []
    expected_first = 0
    for index in range(num_files):
        name_bytes, first, n, digest = DIRECTORY.unpack_from(blob, directory_offset + index * DIRECTORY.size)
        if b'\0' not in name_bytes:
            raise SetDecodeError('SET directory name missing terminator')
        name = name_bytes.split(b'\0', 1)[0].decode('ascii', 'strict')
        if name in seen or first != expected_first or n > count - first:
            raise SetDecodeError('duplicate/overlapping/out-of-order SET ranges')
        source_name_info(name)
        seen.add(name)
        record_bytes = rows_blob[first * SOURCE_RECORD_SIZE:(first + n) * SOURCE_RECORD_SIZE]
        source_bytes = struct.pack('<I', n) + bytes(28) + record_bytes
        if hashlib.sha256(source_bytes).digest() != digest:
            raise SetDecodeError(f'{name}: provenance digest mismatch')
        for j in range(n):
            parse_placement(record_bytes[j * SOURCE_RECORD_SIZE:(j + 1) * SOURCE_RECORD_SIZE])
        level, act, variant = source_name_info(name)
        result_sources.append({'name': name, 'level_id': level, 'act': act, 'variant': variant,
                               'first_record': first, 'record_count': n, 'sha256': digest.hex()})
        expected_first += n
    if expected_first != count:
        raise SetDecodeError('unassigned SET placements')
    return {'files': num_files, 'records': count, 'sources': result_sources,
            'record_size': SOURCE_RECORD_SIZE, 'bytes': len(blob), 'data_offset': rows_offset}
