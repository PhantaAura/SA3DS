#!/usr/bin/env python3
"""Generic strict PVM/PVRT parser for the SADX PC2004 source profile.

The parser intentionally supports only the encodings proven by the currently
available SA1 archives: 38-byte PVMH directory rows, PVRT data format 9
(twiddled mipmaps), and 16-bpp pixel formats ARGB1555/RGB565/ARGB4444.
Unknown encodings fail loudly instead of being guessed.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import hashlib
import io
import struct


class PVMArchiveError(ValueError):
    pass


@dataclass(frozen=True)
class PVMTexture:
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
    wx = code & 0xF
    hy = (code >> 4) & 0xF
    if wx > 7 or hy > 7:
        raise PVMArchiveError(f"unsupported PVM dimension code {code:#x}")
    return 1 << (wx + 2), 1 << (hy + 2)


def _morton2(x: int, y: int) -> int:
    result = 0
    bit = 0
    while (1 << bit) <= max(x, y):
        result |= ((x >> bit) & 1) << (2 * bit)
        result |= ((y >> bit) & 1) << (2 * bit + 1)
        bit += 1
    return result


def _pixel_rgba(value: int, pixel_format: int) -> tuple[int, int, int, int]:
    if pixel_format == 0:  # ARGB1555
        a = 255 if value & 0x8000 else 0
        r5 = (value >> 10) & 0x1F
        g5 = (value >> 5) & 0x1F
        b5 = value & 0x1F
        return r5 * 255 // 31, g5 * 255 // 31, b5 * 255 // 31, a
    if pixel_format == 1:  # RGB565
        r5 = (value >> 11) & 0x1F
        g6 = (value >> 5) & 0x3F
        b5 = value & 0x1F
        return r5 * 255 // 31, g6 * 255 // 63, b5 * 255 // 31, 255
    if pixel_format == 2:  # ARGB4444
        a4 = (value >> 12) & 0xF
        r4 = (value >> 8) & 0xF
        g4 = (value >> 4) & 0xF
        b4 = value & 0xF
        return r4 * 17, g4 * 17, b4 * 17, a4 * 17
    raise PVMArchiveError(f"unsupported PVRT pixel format {pixel_format}")


class PVMArchive:
    DIRECTORY_ROW_SIZE = 38

    def __init__(self, source: bytes | bytearray | memoryview | Path, label: str | None = None):
        if isinstance(source, Path):
            self.label = label or source.name
            self.data = source.read_bytes()
        else:
            self.label = label or "<memory>"
            self.data = bytes(source)
        if len(self.data) < 12 or self.data[:4] != b"PVMH":
            raise PVMArchiveError("missing/truncated PVMH")
        self.header_payload_size = struct.unpack_from("<I", self.data, 4)[0]
        self.flags, self.texture_count = struct.unpack_from("<HH", self.data, 8)
        if self.texture_count <= 0 or self.texture_count > 4096:
            raise PVMArchiveError("implausible PVM texture count")
        self.header_end = 8 + self.header_payload_size
        directory_end = 12 + self.texture_count * self.DIRECTORY_ROW_SIZE
        if self.header_end < directory_end or self.header_end > len(self.data):
            raise PVMArchiveError("PVMH payload does not contain directory")
        # All currently proven SA1 PVMH chunks are 16-byte padded.  Padding is
        # not interpreted; it must be zero so a format extension cannot be
        # silently discarded.
        if self.header_end % 16:
            raise PVMArchiveError("PVMH chunk is not 16-byte aligned")
        if any(self.data[directory_end:self.header_end]):
            raise PVMArchiveError("nonzero/unknown PVMH extension bytes")
        self.entries = self._parse_entries()

    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()

    def _parse_entries(self) -> tuple[PVMTexture, ...]:
        raw_dir = []
        off = 12
        for expected in range(self.texture_count):
            index = struct.unpack_from("<H", self.data, off)[0]
            if index != expected:
                raise PVMArchiveError("PVM directory index discontinuity")
            name = self.data[off + 2:off + 30].split(b"\0", 1)[0].decode("ascii", "strict")
            pf, df, dim, gbix = struct.unpack_from("<BBHI", self.data, off + 30)
            width, height = _decode_dimension(dim)
            raw_dir.append((index, name, pf, df, dim, gbix, width, height))
            off += self.DIRECTORY_ROW_SIZE

        entries = []
        off = self.header_end
        for meta in raw_dir:
            index, name, pf, df, dim, gbix, width, height = meta
            if self.data[off:off + 4] != b"PVRT":
                raise PVMArchiveError(f"texture {index}: missing PVRT")
            payload_size = struct.unpack_from("<I", self.data, off + 4)[0]
            if off + 8 + payload_size > len(self.data):
                raise PVMArchiveError(f"texture {index}: PVRT out of bounds")
            p_pf, p_df, reserved, p_w, p_h = struct.unpack_from("<BBHHH", self.data, off + 8)
            if reserved != 0:
                raise PVMArchiveError(f"texture {index}: nonzero PVRT reserved field")
            if (p_pf, p_df, p_w, p_h) != (pf, df, width, height):
                raise PVMArchiveError(f"texture {index}: directory/PVRT mismatch")
            if df != 9:
                raise PVMArchiveError(f"texture {index}: unsupported data format {df}")
            if pf not in (0, 1, 2):
                raise PVMArchiveError(f"texture {index}: unsupported pixel format {pf}")
            # Exact current SA1 witnesses use one of three payload layouts:
            #   base image only, base image + 16 bytes of zero padding, or
            #   full base-first mip pyramid + 6 bytes of trailing alignment.
            # BEACH01 (non-mip) and BEACH03 (mipped) contain byte-identical
            # m128_cos002 base images, proving the largest mip starts first.
            base_bytes = width * height * 2
            mip_bytes = ((4 * width * height - 1) // 3) * 2
            texel_payload = payload_size - 8
            if texel_payload not in (base_bytes, base_bytes + 16, mip_bytes + 6):
                raise PVMArchiveError(
                    f"texture {index}: unsupported format-9 payload size "
                    f"{texel_payload}; base={base_bytes} mip={mip_bytes + 6}"
                )
            if texel_payload == base_bytes + 16:
                pad_start = off + 16 + base_bytes
                if any(self.data[pad_start:pad_start + 16]):
                    raise PVMArchiveError(f"texture {index}: nonzero base-only padding")
            entries.append(PVMTexture(index, name, pf, df, dim, gbix, width, height, off, payload_size))
            off += 8 + payload_size
        if off != len(self.data):
            raise PVMArchiveError(f"archive trailing bytes: parsed {off}, file {len(self.data)}")
        return tuple(entries)

    def decode_base_rgba(self, index: int) -> bytes:
        e = self.entries[index]
        payload_start = e.pvrt_offset + 16
        full_bytes = e.width * e.height * 2
        # PC2004 format-9 is base-first.  Do not take the tail of a mip chain.
        full = self.data[payload_start:payload_start + full_bytes]
        if len(full) != full_bytes:
            raise PVMArchiveError("truncated largest mip")
        out = bytearray(e.width * e.height * 4)
        for y in range(e.height):
            for x in range(e.width):
                src = _morton2(x, y)
                if src * 2 + 2 > len(full):
                    raise PVMArchiveError("twiddle address outside largest mip")
                value = struct.unpack_from("<H", full, src * 2)[0]
                rgba = _pixel_rgba(value, e.pixel_format)
                dst = (y * e.width + x) * 4
                out[dst:dst + 4] = bytes(rgba)
        return bytes(out)

    def metadata(self) -> dict:
        return {
            "label": self.label,
            "size": len(self.data),
            "sha256": self.sha256(),
            "flags": f"0x{self.flags:04x}",
            "texture_count": self.texture_count,
            "pixel_formats": sorted({e.pixel_format for e in self.entries}),
            "data_formats": sorted({e.data_format for e in self.entries}),
            "textures": [
                {
                    "index": e.index,
                    "name": e.name,
                    "pixel_format": e.pixel_format,
                    "data_format": e.data_format,
                    "global_index": e.global_index,
                    "width": e.width,
                    "height": e.height,
                }
                for e in self.entries
            ],
        }
