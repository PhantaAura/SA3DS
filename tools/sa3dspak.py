#!/usr/bin/env python3
"""Deterministic .sa3dspak v1 packer, validator, inspector, and extractor."""
import argparse
import binascii
import json
import pathlib
import struct
import sys

MAGIC = b"SA3DSPAK"
VERSION = (1, 0)
HEADER = struct.Struct("<8sHHIQQIIII")
ENTRY = struct.Struct("<16sQQII")
HEADER_SIZE = HEADER.size
MAX_CHUNKS = 4096
KNOWN_TYPES = {"GEOMETRY", "COLLISION", "MATERIALS", "TEXTURES", "SET", "CAMERA", "AUDIO", "METADATA"}

class PakError(ValueError):
    pass

def crc32(data):
    return binascii.crc32(data) & 0xFFFFFFFF

def align(value, alignment=16):
    return (value + alignment - 1) & ~(alignment - 1)

def parse(blob):
    if len(blob) < HEADER_SIZE:
        raise PakError("truncated header")
    fields = HEADER.unpack_from(blob)
    magic, major, minor, header_size, file_size, toc_offset, count, flags, payload_crc, stored_header_crc = fields
    if magic != MAGIC: raise PakError("bad magic")
    if major != VERSION[0]: raise PakError("unsupported major version")
    if header_size != HEADER_SIZE: raise PakError("invalid header size")
    if file_size != len(blob): raise PakError("file size mismatch")
    if count > MAX_CHUNKS: raise PakError("too many chunks")
    toc_end = toc_offset + count * ENTRY.size
    if toc_offset < HEADER_SIZE or toc_end > len(blob): raise PakError("TOC out of bounds")
    header_copy = bytearray(blob[:HEADER_SIZE]); header_copy[44:48] = b"\0" * 4
    if crc32(header_copy) != stored_header_crc: raise PakError("header CRC mismatch")
    if crc32(blob[HEADER_SIZE:]) != payload_crc: raise PakError("payload CRC mismatch")
    chunks = []
    for index in range(count):
        raw_type, offset, size, checksum, chunk_flags = ENTRY.unpack_from(blob, toc_offset + index * ENTRY.size)
        chunk_type = raw_type.split(b"\0", 1)[0].decode("ascii", "strict")
        if not chunk_type: raise PakError("empty chunk type")
        if offset < toc_end or offset > len(blob) or size > len(blob) - offset:
            raise PakError("chunk out of bounds: " + chunk_type)
        data = blob[offset:offset + size]
        if crc32(data) != checksum: raise PakError("chunk CRC mismatch: " + chunk_type)
        chunks.append({"type": chunk_type, "offset": offset, "size": size,
                       "crc32": "%08x" % checksum, "flags": chunk_flags, "data": data})
    return {"version": [major, minor], "flags": flags, "file_size": file_size, "chunks": chunks}

def build(chunks):
    if len(chunks) > MAX_CHUNKS: raise PakError("too many chunks")
    toc_offset = HEADER_SIZE
    toc_end = toc_offset + len(chunks) * ENTRY.size
    first_chunk_offset = align(toc_end)
    cursor = first_chunk_offset
    entries, data_parts = [], []
    for chunk_type, data, flags in chunks:
        encoded = chunk_type.encode("ascii")
        if not encoded or len(encoded) > 16: raise PakError("chunk type must be 1..16 ASCII bytes")
        if chunk_type not in KNOWN_TYPES: raise PakError("unknown chunk type: " + chunk_type)
        entries.append(ENTRY.pack(encoded.ljust(16, b"\0"), cursor, len(data), crc32(data), flags))
        data_parts.append(data); cursor += len(data)
        padded = align(cursor) - cursor
        if padded: data_parts.append(b"\0" * padded); cursor += padded
    toc = b"".join(entries)
    padding = b"\0" * (first_chunk_offset - toc_end)
    payload = toc + padding + b"".join(data_parts)
    file_size = HEADER_SIZE + len(payload)
    header = HEADER.pack(MAGIC, *VERSION, HEADER_SIZE, file_size, toc_offset,
                         len(chunks), 0, crc32(payload), 0)
    header_crc = crc32(header)
    header = HEADER.pack(MAGIC, *VERSION, HEADER_SIZE, file_size, toc_offset,
                         len(chunks), 0, crc32(payload), header_crc)
    return header + payload

def command_pack(args):
    chunks = []
    metadata = json.dumps({"format": "sa3dspak", "source_profile": args.source_profile},
                          sort_keys=True, separators=(",", ":")).encode() + b"\n"
    chunks.append(("METADATA", metadata, 0))
    for item in args.chunk:
        if "=" not in item: raise PakError("--chunk requires TYPE=PATH")
        chunk_type, path = item.split("=", 1)
        chunks.append((chunk_type.upper(), pathlib.Path(path).read_bytes(), 0))
    pathlib.Path(args.output).write_bytes(build(chunks))
    print("packed %d chunks -> %s" % (len(chunks), args.output))

def command_validate(args):
    info = parse(pathlib.Path(args.path).read_bytes())
    print("valid .sa3dspak v%d.%d: %d chunks, %d bytes" %
          (*info["version"], len(info["chunks"]), info["file_size"]))

def command_inspect(args):
    info = parse(pathlib.Path(args.path).read_bytes())
    for chunk in info["chunks"]: chunk.pop("data")
    print(json.dumps(info, indent=2, sort_keys=True))

def command_extract(args):
    info = parse(pathlib.Path(args.path).read_bytes())
    output = pathlib.Path(args.output); output.mkdir(parents=True, exist_ok=True)
    for index, chunk in enumerate(info["chunks"]):
        (output / ("%03d_%s.bin" % (index, chunk["type"]))).write_bytes(chunk["data"])
    print("extracted %d chunks -> %s" % (len(info["chunks"]), output))

def main(argv=None):
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("pack"); p.add_argument("output"); p.add_argument("--source-profile", required=True); p.add_argument("--chunk", action="append", default=[]); p.set_defaults(function=command_pack)
    for name, function in (("validate", command_validate), ("inspect", command_inspect)):
        p = sub.add_parser(name); p.add_argument("path"); p.set_defaults(function=function)
    p = sub.add_parser("extract"); p.add_argument("path"); p.add_argument("output"); p.set_defaults(function=command_extract)
    args = parser.parse_args(argv)
    try: args.function(args)
    except (OSError, PakError, UnicodeError) as error:
        print("error: %s" % error, file=sys.stderr); return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
