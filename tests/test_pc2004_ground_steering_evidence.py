#!/usr/bin/env python3
"""Read-only exact-owner PC2004 steering witnesses; not a retail execution test."""
from pathlib import Path
import hashlib
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader

# Sizes are end-exclusive function ranges, excluding alignment padding.
FUNCTIONS = {
    (0x443C50, 0x19A): '7f0aae2f159f38100768623cb99a8d3c6c2c578123700865ad7bc45d23942916',
    (0x443DF0, 0x67): '9699dc16e2f1e7b153cb290936a5fd0007db2f35d1a2449a5adb8b9d65c1d61e',
    (0x443E60, 0xE7): 'f0d8e1380aa89174d29dbf93a0c89eb40605b2f6ec8a56c3ab9184cc4b69e52b',
    (0x44BB60, 0x20A): '71b1245d6df926e3603a8c1f5ee358e08919ee005d6ada147ecf9c1f675e87cb',
}

SIGNATURES = {
    'general Up comparison is float32': (0x443D5F, 'd81d50d57d00'),
    'slow Up comparison is float32': (0x443EE8, 'd81d50d57d00'),
    'general retained one-minus-weight': (0x443DB0, 'd90510cb7d005dd8e1'),
    'quick writes transformed local speed': (0x443E47, '891689460483c41c894e08'),
    'slow float argument then sqrt/multiply/truncate':
        (0x443EAE, 'd91c24e83a333400d80d68ce7d00e87f042000'),
    'Up cutoff float32 +0.4': (0x7DD550, 'cdcccc3e'),
    'general/slow old weight float32 0.99': (0x7E0118, 'a4707d3f'),
    'slow transformed weight float32 1-0.99': (0x7E011C, '00d7233c'),
    'general no-input old weight float32 0.05': (0x7E0070, 'cdcc4c3d'),
    'steep old/transformed weight float32 0.5': (0x7DCC98, '0000003f'),
}

CALLS = {
    'PGetAcceleration quick': (0x44C81E, 0x443DF0),
    'PGetAcceleration ordinary slow': (0x44C889, 0x443E60),
    'PGetAcceleration flagged slow': (0x44C8B8, 0x443E60),
    'PGetAcceleration general': (0x44C904, 0x443C50),
    'ball PGetRotation flagged slow': (0x44BD38, 0x443E60),
    'ball PGetRotation general': (0x44BD5A, 0x443C50),
    'Sonic charge rotation': (0x49AE32, 0x44BB60),
    'Sonic roll rotation': (0x49AEBE, 0x44BB60),
}


def verify(path):
    read = verified_reader(path)
    for (address, size), digest in FUNCTIONS.items():
        if hashlib.sha256(read(address, size)).hexdigest() != digest:
            raise ValueError(f'steering function digest mismatch at {address:#x}')
    for label, (address, expected) in SIGNATURES.items():
        expected = bytes.fromhex(expected)
        if read(address, len(expected)) != expected:
            raise ValueError('steering instruction/constant mismatch: ' + label)
    for label, (address, target) in CALLS.items():
        instruction = read(address, 5)
        if instruction[0] != 0xE8 or address + 5 + int.from_bytes(instruction[1:], 'little', signed=True) != target:
            raise ValueError('steering call target mismatch: ' + label)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: test_pc2004_ground_steering_evidence.py OWNER_SONIC_EXE')
    verify(sys.argv[1])
    print('PC2004 ground steering evidence: PASS (quick/general/slow call paths, float32 Up threshold, retained blend weights, ball general path; static only)')
