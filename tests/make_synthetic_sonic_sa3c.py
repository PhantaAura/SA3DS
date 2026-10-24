#!/usr/bin/env python3
"""Generate a tiny original test fixture; contains NO game assets."""
import argparse
import struct
from pathlib import Path

TAGS = (b'NODE', b'MODL', b'MESH', b'VRTX', b'INDX', b'MATL',
        b'TEXS', b'ANIM', b'CHAN', b'KEYS')
STRIDES = (48, 32, 24, 32, 2, 32, 24, 24, 24, 16)
COUNTS = (148, 0, 0, 0, 0, 0, 0, 124, 1, 7)

ACTION_METADATA = {
    1: (72, 32), 3: (72, 32), 4: (72, 32), 5: (72, 32),
    6: (72, 32), 18: (72, 33), 14: (146, 7), 8: (72, 32),
    9: (72, 33), 10: (72, 32), 11: (72, 16), 7: (72, 33),
    22: (72, 16), 20: (72, 16), 19: (72, 16), 21: (147, 7),
    122: (72, 20), 123: (72, 16),
    120: (72, 20), 121: (72, 39),
}

def align16(n):
    return (n + 15) & ~15

def build(bad_key=False):
    sections = []
    size = 272
    for tag, stride, count in zip(TAGS, STRIDES, COUNTS):
        size = align16(size)
        sections.append((tag, count, size, stride, count * stride))
        size += count * stride
    data = bytearray(size)
    data[:4] = b'SA3C'
    struct.pack_into('<IIII', data, 8, 272, 10, 0, size)
    for i, (tag, count, offset, stride, length) in enumerate(sections):
        struct.pack_into('<4sIIII', data, 64 + i * 20,
                         tag, count, offset, stride, length)
    node = sections[0][2]
    for i in range(COUNTS[0]):
        struct.pack_into('<HHH', data, node + i * STRIDES[0],
                         0xffff, 0xffff, 0xffff)
        struct.pack_into('<fff', data, node + i * STRIDES[0] + 36, 1, 1, 1)
    anim = sections[7][2]
    for i in range(COUNTS[7]):
        root, frames = ACTION_METADATA.get(i, (72, 11))
        struct.pack_into('<HHHHII', data, anim + i * 24,
                         0, root, frames, 0, 0, 1)
    chan = sections[8][2]
    struct.pack_into('<HHIHHI', data, chan, 72, 3, 0, 5, 2, 5)
    keys = sections[9][2]
    for i, frame in enumerate((0, 1, 2, 3, 10)):
        x = float('nan') if bad_key and i == 1 else float(frame)
        struct.pack_into('<Ifff', data, keys + i * 16, frame, x, 0, 0)
    struct.pack_into('<Ifff', data, keys + 5 * 16, 0,
                     -170.0 * 3.141592653589793 / 180.0, 0, 0)
    struct.pack_into('<Ifff', data, keys + 6 * 16, 2,
                     170.0 * 3.141592653589793 / 180.0, 0, 0)
    return bytes(data)

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('output', type=Path)
    p.add_argument('--bad-key', action='store_true')
    args = p.parse_args()
    args.output.write_bytes(build(args.bad_key))
