#!/usr/bin/env python3
"""Malformed package cases; fixtures are generated and contain no game data."""
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
import sa3dspak

def rejected(blob):
    try: sa3dspak.parse(bytes(blob))
    except sa3dspak.PakError: return
    raise AssertionError("malformed package was accepted")

def refresh_header(blob):
    struct.pack_into("<I", blob, 40, sa3dspak.crc32(blob[sa3dspak.HEADER_SIZE:]))
    struct.pack_into("<I", blob, 44, 0)
    struct.pack_into("<I", blob, 44, sa3dspak.crc32(blob[:sa3dspak.HEADER_SIZE]))

def main():
    valid = bytearray(sa3dspak.build([("METADATA", b"{}\n", 0)]))
    sa3dspak.parse(valid)
    rejected(valid[:20])
    bad = bytearray(valid); bad[0] ^= 1; rejected(bad)
    bad = bytearray(valid); struct.pack_into("<H", bad, 8, 99); refresh_header(bad); rejected(bad)
    bad = bytearray(valid); struct.pack_into("<Q", bad, 16, len(bad) + 1); refresh_header(bad); rejected(bad)
    bad = bytearray(valid); bad[-1] ^= 1; rejected(bad)
    bad = bytearray(valid); struct.pack_into("<I", bad, 32, sa3dspak.MAX_CHUNKS + 1); refresh_header(bad); rejected(bad)
    bad = bytearray(valid); struct.pack_into("<Q", bad, sa3dspak.HEADER_SIZE + 16, len(bad) + 1); refresh_header(bad); rejected(bad)
    bad = bytearray(valid); struct.pack_into("<I", bad, sa3dspak.HEADER_SIZE + 32, 0); refresh_header(bad); rejected(bad)
    print("8 malformed package cases: PASS")

if __name__ == "__main__": main()
