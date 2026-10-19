#!/usr/bin/env python3
"""Strict parser/decoder for the exact SADX Sonic PVM texture archive.

The checkpoint does not include the archive or decoded Sega textures.  This
module operates on the owner's local source file and exposes metadata / RGBA
pixels to the importer at build time.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import hashlib
import json
import struct

SONIC_PVM_SIZE = 155_776
SONIC_PVM_SHA256 = "f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe"


class PVMError(ValueError):
    pass


@dataclass(frozen=True)
class PVMEntry:
    index: int
    name: str
    pixel_format: int
    data_format: int
    dimension_code: int
    global_index: int
    width: int
    height: int
    pvrt_offset: int
    pvrt_payload_size: int


def _decode_dimension(code: int) -> tuple[int, int]:
    # Exact archive witness: 0x11->8x8, 0x22->16x16, 0x33->32x32,
    # 0x44->64x64, 0x55->128x128.  Each nibble is log2(size)-2.
    w_exp = code & 0xF
    h_exp = (code >> 4) & 0xF
    return 1 << (w_exp + 2), 1 << (h_exp + 2)


class SonicPVM:
    def __init__(self, path: Path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        digest = hashlib.sha256(self.data).hexdigest()
        if len(self.data) != SONIC_PVM_SIZE or digest != SONIC_PVM_SHA256:
            raise PVMError(
                "unsupported SONIC.PVM: expected exact source "
                f"size={SONIC_PVM_SIZE} sha256={SONIC_PVM_SHA256}; got "
                f"size={len(self.data)} sha256={digest}"
            )
        if self.data[:4] != b"PVMH":
            raise PVMError("missing PVMH")
        self.header_payload_size = struct.unpack_from("<I", self.data, 4)[0]
        self.flags, self.texture_count = struct.unpack_from("<HH", self.data, 8)
        if self.flags != 0x000F or self.texture_count != 16:
            raise PVMError("unexpected Sonic PVM header flags/count")
        self.entries = self._parse_entries()

    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()

    def _parse_entries(self) -> list[PVMEntry]:
        directory = []
        off = 12
        raw_directory = []
        for _ in range(self.texture_count):
            index = struct.unpack_from("<H", self.data, off)[0]
            name = self.data[off + 2:off + 30].split(b"\0", 1)[0].decode("ascii", "replace")
            pixel_format, data_format, dimension_code, global_index = struct.unpack_from("<BBHI", self.data, off + 30)
            width, height = _decode_dimension(dimension_code)
            raw_directory.append((index, name, pixel_format, data_format, dimension_code, global_index, width, height))
            off += 38

        # Header chunk begins at 0 and is 8 + payload bytes long.
        pvrt = 8 + self.header_payload_size
        for expected_index, meta in enumerate(raw_directory):
            index, name, pixel_format, data_format, dimension_code, global_index, width, height = meta
            if index != expected_index:
                raise PVMError("Sonic PVM directory index discontinuity")
            if self.data[pvrt:pvrt + 4] != b"PVRT":
                raise PVMError(f"texture {index}: missing PVRT")
            payload_size = struct.unpack_from("<I", self.data, pvrt + 4)[0]
            if pvrt + 8 + payload_size > len(self.data):
                raise PVMError("PVRT extends beyond archive")
            pf, df, reserved, pvrt_w, pvrt_h = struct.unpack_from("<BBHHH", self.data, pvrt + 8)
            if reserved != 0:
                raise PVMError("unexpected PVRT reserved field")
            if (pf, df, pvrt_w, pvrt_h) != (pixel_format, data_format, width, height):
                raise PVMError(
                    f"texture {index}: directory/PVRT mismatch: "
                    f"dir={(pixel_format,data_format,width,height)} pvrt={(pf,df,pvrt_w,pvrt_h)}"
                )
            # Exact Sonic archive uses twiddled mipmaps (data format 9).
            if data_format != 9:
                raise PVMError(f"texture {index}: unsupported data format {data_format}")
            expected_texel_bytes = ((4 * width * height - 1) // 3) * 2
            # All exact Sonic PVRTs contain six bytes of mip-chain alignment
            # beyond the complete 16-bpp mip pyramid.
            if payload_size - 8 != expected_texel_bytes + 6:
                raise PVMError(
                    f"texture {index}: unexpected mip payload size {payload_size - 8}"
                )
            directory.append(PVMEntry(
                index=index,
                name=name,
                pixel_format=pixel_format,
                data_format=data_format,
                dimension_code=dimension_code,
                global_index=global_index,
                width=width,
                height=height,
                pvrt_offset=pvrt,
                pvrt_payload_size=payload_size,
            ))
            pvrt += 8 + payload_size
        if pvrt != len(self.data):
            raise PVMError(f"archive trailing bytes: parsed {pvrt}, file {len(self.data)}")
        return directory

    @staticmethod
    def _morton2(x: int, y: int) -> int:
        result = 0
        bit = 0
        while (1 << bit) <= max(x, y):
            result |= ((x >> bit) & 1) << (2 * bit)
            result |= ((y >> bit) & 1) << (2 * bit + 1)
            bit += 1
        return result

    @staticmethod
    def _pixel_rgba(value: int, pixel_format: int) -> tuple[int, int, int, int]:
        if pixel_format == 0:  # ARGB1555
            a = 255 if value & 0x8000 else 0
            r5 = (value >> 10) & 0x1F
            g5 = (value >> 5) & 0x1F
            b5 = value & 0x1F
            return (r5 * 255 // 31, g5 * 255 // 31, b5 * 255 // 31, a)
        if pixel_format == 1:  # RGB565
            r5 = (value >> 11) & 0x1F
            g6 = (value >> 5) & 0x3F
            b5 = value & 0x1F
            return (r5 * 255 // 31, g6 * 255 // 63, b5 * 255 // 31, 255)
        if pixel_format == 2:  # ARGB4444
            a4 = (value >> 12) & 0xF
            r4 = (value >> 8) & 0xF
            g4 = (value >> 4) & 0xF
            b4 = value & 0xF
            return (r4 * 17, g4 * 17, b4 * 17, a4 * 17)
        raise PVMError(f"unsupported Sonic pixel format {pixel_format}")

    def decode_base_rgba(self, index: int) -> bytes:
        """Return only the largest mip as row-major RGBA8.

        This is for importer/runtime conversion and validation; the checkpoint
        never bundles the returned Sega pixel payload.
        """
        entry = self.entries[index]
        payload_start = entry.pvrt_offset + 16  # chunk header + 8-byte PVRT texture header
        full_bytes = entry.width * entry.height * 2
        # 0.0.9.1B correction: PC2004 format-9 stores the largest mip first.
        # Cross-archive BEACH01/BEACH03 witnesses prove identical base bytes
        # at the start of both non-mipped and mipped payloads.
        full = self.data[payload_start:payload_start + full_bytes]
        if len(full) != full_bytes:
            raise PVMError("truncated largest mip")
        out = bytearray(entry.width * entry.height * 4)
        for y in range(entry.height):
            for x in range(entry.width):
                source_index = self._morton2(x, y)
                value = struct.unpack_from("<H", full, source_index * 2)[0]
                rgba = self._pixel_rgba(value, entry.pixel_format)
                dst = (y * entry.width + x) * 4
                out[dst:dst + 4] = bytes(rgba)
        return bytes(out)

    def report(self) -> dict:
        return {
            "schema": "sa3ds-pc2004-sonic-pvm-report-v1",
            "source": {
                "size": len(self.data),
                "sha256": self.sha256(),
            },
            "flags": self.flags,
            "texture_count": self.texture_count,
            "textures": [
                {
                    "index": e.index,
                    "name": e.name,
                    "pixel_format": e.pixel_format,
                    "data_format": e.data_format,
                    "width": e.width,
                    "height": e.height,
                    "global_index": e.global_index,
                    "pvrt_payload_size": e.pvrt_payload_size,
                }
                for e in self.entries
            ],
        }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pvm", type=Path)
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()
    pvm = SonicPVM(args.pvm)
    report = pvm.report()
    text = json.dumps(report, indent=2)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
