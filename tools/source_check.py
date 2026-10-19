#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import sys

STEAM_EXE = "Sonic Adventure DX.exe"
STEAM_SIZE = 90922000
STEAM_SHA256 = "e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6"

def load_manifest(path):
    lines = pathlib.Path(path).read_text(encoding="utf-8").splitlines()
    if len(lines) < 2 or lines[1] != "SHA256\tSIZE\tPATH": raise ValueError("unrecognized manifest header")
    entries = {}
    for number, line in enumerate(lines[2:], 3):
        parts = line.split("\t", 2)
        if len(parts) != 3 or len(parts[0]) != 64: raise ValueError("malformed row %d" % number)
        entries[parts[2]] = (parts[0], int(parts[1]))
    return entries

def main():
    parser = argparse.ArgumentParser(); parser.add_argument("manifest"); parser.add_argument("--root")
    args = parser.parse_args()
    try: entries = load_manifest(args.manifest)
    except (OSError, ValueError) as error: print("source-check: %s" % error, file=sys.stderr); return 1
    if entries.get(STEAM_EXE) != (STEAM_SHA256, STEAM_SIZE):
        print("source-check: manifest is not the supported clean Steam profile", file=sys.stderr); return 1
    print("source-check: supported Steam inventory (%d entries)" % len(entries))
    if not args.root: return 0
    root = pathlib.Path(args.root); mismatches = 0
    for relative, (expected_hash, expected_size) in entries.items():
        path = root / relative
        if not path.is_file() or path.stat().st_size != expected_size:
            print("MISMATCH size/missing: " + relative); mismatches += 1; continue
        hasher = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""): hasher.update(block)
        digest = hasher.hexdigest()
        if digest != expected_hash: print("MISMATCH hash: " + relative); mismatches += 1
    print("source-check: %d mismatch(es)" % mismatches)
    return 1 if mismatches else 0

if __name__ == "__main__": raise SystemExit(main())
