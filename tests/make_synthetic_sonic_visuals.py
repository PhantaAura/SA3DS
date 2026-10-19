#!/usr/bin/env python3
"""Create original, Sega-free SA3C/SA3T integration fixtures."""
import argparse
import struct
from pathlib import Path

TAGS = (b'NODE', b'MODL', b'MESH', b'VRTX', b'INDX', b'MATL',
        b'TEXS', b'ANIM', b'CHAN', b'KEYS')
STRIDES = (48, 32, 24, 32, 2, 32, 24, 24, 24, 16)
COUNTS = (1, 1, 1, 3, 3, 1, 1, 23, 1, 5)
ORIGIN = bytes(range(32))

def align(n):
    return (n + 15) & ~15

def make_sa3c(bad_uv=False, bad_metadata=False, bad_index=False, mismatch=False):
    sections = []
    end = 272
    for tag, stride, count in zip(TAGS, STRIDES, COUNTS):
        end = align(end)
        sections.append((tag, count, end, stride, count * stride))
        end += count * stride
    b = bytearray(end)
    b[:4] = b'SA3C'
    struct.pack_into('<IIII', b, 8, 272, 10, 0, end)
    b[24:56] = bytes([255]) * 32 if mismatch else ORIGIN
    for i, section in enumerate(sections):
        struct.pack_into('<4sIIII', b, 64 + i * 20, *section)
    off = [section[2] for section in sections]
    struct.pack_into('<HHH', b, off[0], 0, 0xffff, 0xffff)
    struct.pack_into('<fff', b, off[0] + 36, 1, 1, 1)
    struct.pack_into('<IIIIII', b, off[1], 0, 1, 0, 3, 0, 3)
    struct.pack_into('<HHIIH', b, off[2], 0, 0, 0, 3, 0xC000)
    for i in range(3):
        v = [float(i), 0, 0, 0, 0, 1, float(i*64), 0]
        if bad_uv and i == 1:
            v[6] = float('nan')
        struct.pack_into('<8f', b, off[3] + 32*i, *v)
    struct.pack_into('<3H', b, off[4], 0, 1, 3 if bad_index else 2)
    struct.pack_into('<I', b, off[5], 0xffe0c0a0)
    struct.pack_into('<H', b, off[5]+12, 0)
    struct.pack_into('<IIHHHH', b, off[6], 0, 7, 2, 3, 16 if bad_metadata else 8, 8)
    for i in range(23):
        struct.pack_into('<HHHHII', b, off[7]+24*i, 0, 0, 11, 0, 0, 1)
    struct.pack_into('<HHIH', b, off[8], 0, 1, 0, 5)
    for i, frame in enumerate((0, 1, 2, 3, 10)):
        struct.pack_into('<Ifff', b, off[9]+16*i, frame, float(frame), 0, 0)
    return bytes(b)

def make_sa3t():
    b = bytearray(96+8*8*4)
    b[:4] = b'SA3T'
    struct.pack_into('<III', b, 8, 64, 1, len(b))
    b[20:52] = ORIGIN
    struct.pack_into('<HHHHIIHHI', b, 64, 8, 8, 1, 0, 96, 256, 2, 3, 7)
    for i in range(64):
        b[96+4*i:100+4*i] = bytes((i, 255-i, 50, 127 if i == 0 else 255))
    return bytes(b)

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('folder', type=Path)
    for name in ('bad_uv', 'bad_metadata', 'bad_index', 'mismatch'):
        ap.add_argument('--'+name.replace('_','-'), action='store_true')
    a = ap.parse_args()
    a.folder.mkdir(parents=True, exist_ok=True)
    (a.folder/'sonic.sa3c').write_bytes(make_sa3c(a.bad_uv, a.bad_metadata, a.bad_index, a.mismatch))
    (a.folder/'sonic.sa3t').write_bytes(make_sa3t())
