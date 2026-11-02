#!/usr/bin/env python3
"""Exact-owner spring/CCL/task witnesses, not live retail execution evidence."""
import hashlib
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
# End-exclusive inspected code ranges; some include alignment/jump-table bytes.
RANGES = {
    (0x41b9f0, 0x59): '29d45ea2c2bc170c9590c3a87fa57c138560b8cd8b66723ee94b8cf49e571965',
    (0x41c6e0, 0x70): '3219199a9fe64a6e95fce60e4887326c586733c082b9fa5e4673b157dd753623',
    (0x41cbc0, 0x20): 'bd8fb4a2347f394868bfeeeb34ccb6bc44819b1ca6dbfa6a3a4afd49c93dc51a',
    (0x441750, 0x22): 'd1ceeb0403ea0777bcccdd91a5de0d5bb83486a2bad64de42bb8a287f7680a12',
    (0x446f10, 0x3b): '7049f4662a1b2264edd308e7669d2931ea6fad43d0611534cf8fd82951f4f043',
    (0x43ee70, 0x1a0): 'd4ee73b36ca05a5f331fc78b5f8f8e8224312ecdb680e127a86802b6ba66a3f8',
    (0x449500, 0x1300): '47364b7cdff995e662cbe91b144ac59580957b8f2e4b7e111b53c8ac12a1026c',
    (0x44c2e0, 0x7c): '2abab1269c8134e97624aaa635a12fdac65da519971e5172f710a3c7218976ce',
    (0x7a4c30, 0x220): 'a0d573491887ea3abd6dfa7a313a33edadf437d292b61f42abaa3d68f4348b52',
    (0x7a4e50, 0x240): 'd7a8f042a99a52ea9136cd810c001fbc85b6cecbc201a0183147d1f2f2077e67',
    (0x41aa00, 0x1e0): '90e4a2219a220adea040ad0b52cfaa18537774e19474286f332acea5d6af4cdb',
    (0x41ccd0, 0x3e0): 'f44c13321a76eebfc889323af6593592d5cc2762271cee9afae0ed1235a15e90',
    (0x41fd00, 0xa0): '388c59b0208893bda0a121082e154f02162ff45f4e099fa9ef72b244c418d3ea',
    (0x41fec0, 0x150): '8aa472970d8d22db778ff5cf96fd70465e88b8d1b90cdba84a68955978e323ef',
    (0x4432e0, 0x370): 'b73fdc64b6fc33185eecddceb1f046873cb22a0a7173761860e2f9262088ceeb',
    (0x495fa0, 0x814): '1c7c385b07865306b378c28c552efe4b10afb78faee6b19628dfb3fd4d0f5bf8',
}
SIGNATURES = {
    'tail external force clear': (0x446f2a, '897818897814897810'),
    'grounded local normal speed clear': (0x43eecc, '668b4604662503007408c744241000000000'),
    'action selector indexes action minus one': (0x497017, '0fbe45004883f856'),
    'movement indexes direct action': (0x49ab1d, '0fbe0383f857'),
    'command3 action9 and high status clear': (0x495fff, 'c60709e849fbffff806705da'),
    'command4 action10': (0x496046, 'c6070ae802fbffff'),
    'command4 conditional ball clear': (0x4965ba, '8bc881e1000100006685c97409'),
    'command4 retained-low-status write': (0x4965c7, '25fffaffff66894704806705df'),
    'springB control timer write': (0x441403, '6689510a'),
    'action10 local normal versus nocon_speed': (0x49af7e, 'd9453c55d89db8000000'),
    'sphere distance f32 boundary': (0x41aa61, 'd95c2450'),
    'default gravity axis3': (0x41c6cc, 'c7058c32b20303000000'),
    'springB initial cooldown10': (0x7a4eb9, 'c646030a'),
    'springB post-hit phase': (0x7a5026, 'c74608cdcccc3d'),
    'springB delayed launch cooldown5': (0x7a5043, '6a018bc5c6460305'),
}
CALLS = {
    'action9 air control': (0x49af74, 0x44b9c0),
    'action10 high normal no-input': (0x49af95, 0x4432e0),
    'action10 low normal air control': (0x49b33e, 0x44b9c0),
    'sphere primary center1': (0x41aa25, 0x419160),
    'sphere primary center2': (0x41aa34, 0x419160),
    'form7 cylinder world-axis center': (0x41cd18, 0x41c5d0),
}
def verify(path):
    read=verified_reader(path)
    for (address,size),digest in RANGES.items():
        assert hashlib.sha256(read(address,size)).hexdigest()==digest,hex(address)
    for label,(address,hx) in SIGNATURES.items():
        expected=bytes.fromhex(hx)
        assert read(address,len(expected))==expected,label
    for label,(address,target) in CALLS.items():
        ins=read(address,5)
        assert ins[0]==0xe8 and address+5+int.from_bytes(ins[1:],'little',signed=True)==target,label
    assert struct.unpack('<2I',read(0x49a850+8*4,8))==(0x4977f6,0x497881)
    assert struct.unpack('<2I',read(0x49bfbc+9*4,8))==(0x49af71,0x49af7e)
    assert struct.unpack('<2I',read(0x4967b4+2*4,8))==(0x495fff,0x496046)
    assert read(0x38cd440,8)==bytes.fromhex('0001f0e000000000')
    assert struct.unpack('<2f',read(0x38cd440+0x14,8))==(6.,10.)
    assert read(0x38cd470,8)==bytes.fromhex('0000f0e000000000')
    assert struct.unpack('<f',read(0x38cd470+0x14,4))==(14.,)
    assert read(0x91cbf0,8)==bytes.fromhex('000706e020040000')
    assert struct.unpack('<3f',read(0x91cbf0+0x14,12))==(4.,4.,struct.unpack('<f',struct.pack('<f',.65))[0])
    assert struct.unpack('<I',read(0x90e3d4+4*4,4))[0]==0x100000
    secondary_attributes=struct.unpack('<I',read(0x91cc20+4,4))[0]
    assert secondary_attributes&0x100000  # list4 excludes this attack record
    assert struct.unpack('<f',read(0x7dd584,4))[0]==struct.unpack('<f',struct.pack('<f',.6))[0]
    assert struct.unpack('<f',read(0x7dd550,4))[0]==struct.unpack('<f',struct.pack('<f',.4))[0]
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: test_pc2004_spring_runtime_evidence.py OWNER_SONIC_EXE')
    verify(sys.argv[1])
    print('PC2004 spring runtime evidence: PASS (task/command/dispatch/CCL ranges, witnesses and source collision records; static only)')
