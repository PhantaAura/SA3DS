#!/usr/bin/env python3
"""Owner-only exact-PC2004 movement/display orientation witnesses; no payload output."""
import argparse
import hashlib
from pathlib import Path
from verify_pc2004_collision_work import PROFILE_SHA256, parse_pe, va_to_off

SIGNATURES = {
    # Both paths apply task Z/X rotation before Y, using the same Ninja helpers.
    "movement Z rotation": (0x43ECCD, "e8de623400"),
    "movement X rotation": (0x43ECDE, "e8fd613400"),
    "movement Y negation": (0x43ECE3, "8b4618f7d87411"),
    "movement Y argument/call": (0x43ECEA, "668b4e1866f7d90fb7d133c9e825613400"),
    "display Z rotation": (0x494A2B, "e880052f00"),
    "display X rotation": (0x494A3C, "e89f042f00"),
    "display half-turn minus task Y": (0x494A41, "8b4f18b8008000002bc17412"),
    "display Y argument/call": (0x494A4D, "66b90080662b4f180fb7d133c9e8c1032f00"),
}

def verify(path):
    image=Path(path).read_bytes()
    if hashlib.sha256(image).hexdigest()!=PROFILE_SHA256:
        raise ValueError("unsupported PC2004 executable identity")
    base,sections=parse_pe(image)
    for label,(va,hx) in SIGNATURES.items():
        offset,_=va_to_off(va,base,sections)
        expected=bytes.fromhex(hx)
        if image[offset:offset+len(expected)]!=expected:
            raise ValueError("instruction witness mismatch: "+label)
    print("PC2004 player facing: PASS (exact EXE; movement -taskY, display 0x8000-taskY; half-turn about local Up)")

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pc2004_exe")
    verify(parser.parse_args().pc2004_exe)
