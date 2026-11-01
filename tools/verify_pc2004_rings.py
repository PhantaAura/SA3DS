#!/usr/bin/env python3
"""Exact-owner static witnesses for the bounded native direct-ring implementation.

No retail trace or complete object/collision parity is established by this tool.
Only short instruction witnesses and hashes are retained; owner assets/decompiled
bodies are not emitted. Values are read from the privately supplied executable.
"""
from pathlib import Path
import argparse
import configparser
import hashlib
import json
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader

# LLVM disassembly corroborated against the exact supported owner executable.
SIGNATURES={
    'Sonic two collision shapes':(0x49ABC1,'6a0268f0cb9100'),
    'Sonic radius from parameter rad':(0x49ABDA,'8b950c010000895114'),
    'Sonic current center_height':(0x4915F6,'8b911c010000'),
    'Sonic center full frame conversion':(0x49161A,'e871d6faff'),
    'Sonic center task XYZ addition':(0x491638,'d94720d80083c408d918d94724d8460cd95e0cd94728d84610d95e10'),
    'Sonic per-tick center call':(0x49BEFA,'e8f156ffff'),
    'ring collision init':(0x4503AC,'6a076a0168106b910053e835c7fcff'),
    'ring initial state0 to4':(0x4503A0,'c60604'),
    'ring state4 to5':(0x4503A6,'c60605'),
    'ring state5 to1':(0x45041D,'c60601'),
    'prior collision flag gate':(0x450466,'8b5638f64204010f84f3000000'),
    'collection consumes ring before tally':(0x450473,'6a00c60602e88316ffff'),
    'ring eligibility powerup flag':(0x45047D,'0fbf40062500400000'),
    'primary Sonic tally and sound7':(0x4504CB,'6a01e80e57fdff6a006a006a006a07e89138fdff'),
    'ring collision enqueue':(0x45057A,'56e800bdfcff'),
    'ring yaw increment364':(0x450586,'056c010000894618'),
    'ring pickup removal':(0x4506B9,'e892ba0100'),
    'SET consumed eligibility and active clear':(0x46C157,'8061037f8b481c806102fe'),
    'sphere dispatch call':(0x41FB03,'50565357e8f4aeffff'),
    'strict sphere overlap branch':(0x41AA7F,'ded9dfe0ddd8f6c4050f8a3a010000'),
    'signed16 ring addition':(0x425BE0,'668b0de4f0b003668bc166034c2404'),
    'ring counter write and sign extension':(0x425C03,'66890de4f0b0030fbfc9'),
    'hundreds increment lives only upwards':(0x425C21,'3bf27d0b2bd652e833ffffff'),
    'lives signed byte overflow saturates127':(0x425B7E,'001d34efb003790c6685db7e07c60534efb0037f'),
    'normal SET mode':(0x422961,'a0b48db10384c07407a144f49000eb0c'),
    'normal SET suffix by character':(0x422971,'e86a1bffff8b048524f49000'),
    'current character getter':(0x4144E0,'0fb705c02db203c3'),
    'SET class low12':(0x46BB0A,'81e1ff0f0000'),
    'SET clip selector high3':(0x46BB18,'c1e80c83e007'),
    'SET clip selector rules':(0x46BB21,'741b48a1f0dcab03740a83f8027c0ee98801000083f8010f8d7f010000'),
    'clip zero register':(0x4025D1,'33ed'),
    'clip getter default1 register':(0x4025FC,'bb01000000'),
    'clip key and section with default':(0x402790,'6890ca7d00536834ca7d00687cca7d00ffd6'),
    'clip clamp0..2':(0x4027A2,'3bc5a3f0dcab037d0433c0eb0a83f8027e0ab802000000a3f0dcab03'),
}
ADD_RINGS_SHA256='e264dcbdc4316e998eb21756f458840cddb72122e9b74c461e496c86daf2e809'

def verify(path):
    read=verified_reader(path)
    for label,(address,hexbytes) in SIGNATURES.items():
        expected=bytes.fromhex(hexbytes)
        if read(address,len(expected))!=expected:
            raise ValueError('ring instruction witness mismatch: '+label)
    # Covers the complete bounded function, including absence of a999 clamp.
    if hashlib.sha256(read(0x425BE0,0x425C4D-0x425BE0)).hexdigest()!=ADD_RINGS_SHA256:
        raise ValueError('AddRings bounded function identity mismatch')
    def u32(a):return struct.unpack('<I',read(a,4))[0]
    def f32(a):return struct.unpack('<f',read(a,4))[0]
    if (u32(0x41FBC8+7*4),u32(0x41FBEC))!=(0x41FA57,0x41FB03):
        raise ValueError('form7 versus form0 dispatch mismatch')
    if (f32(0x9154E8+28*4),f32(0x9154E8+32*4))!=(4.0,struct.unpack('<f',struct.pack('<f',5.4))[0]):
        raise ValueError('Sonic collision radius/center parameters mismatch')
    if read(0x91CBF0+4,4)!=struct.pack('<I',0x420):
        raise ValueError('Sonic primary collision world-center attributes mismatch')
    if read(0x916B10+8,12)!=bytes(12) or f32(0x916B10+20)!=5.0:
        raise ValueError('ring collision sphere mismatch')
    if u32(0x90F424)!=0x7DDEBC or read(0x7DDEBC,2)!=b'S\0':
        raise ValueError('normal Sonic SET suffix mismatch')
    if read(0x7DCA34,10)!=b'cliplevel\0' or read(0x7DCA7C,8)!=b'sonicDX\0':
        raise ValueError('source clipping setting name mismatch')
    return {'instruction_witnesses':len(SIGNATURES),'sonic_radius':4.0,
            'sonic_center_height':f32(0x9154E8+32*4),'ring_radius':5.0,
            'normal_sonic_set_suffix':'S','cliplevel_fallback':1,
            'authority':'EXACT-OWNER STATIC INSTRUCTIONS/DATA; NOT RETAIL TRACE'}

def ini_clip(path):
    """Report this owner's readable integer setting; do not alter their INI."""
    parser=configparser.ConfigParser()
    if not path.is_file():return {'path':str(path),'present':False,'cliplevel':1}
    parser.read_string(path.read_text(encoding='utf-8-sig'))
    section=next((name for name in parser.sections() if name.lower()=='sonicdx'),None)
    value=parser.get(section,'cliplevel',fallback='1') if section else '1'
    try:value=int(value,10)
    except ValueError:raise ValueError('owner cliplevel is not a supported integer')
    return {'path':str(path),'present':True,'cliplevel':min(2,max(0,value))}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe',type=Path)
    parser.add_argument('--ini',type=Path)
    args=parser.parse_args()
    report=verify(args.sonic_exe)
    report['owner_ini']=ini_clip(args.ini or args.sonic_exe.parent/'sonicDX.ini')
    print('PC2004 direct rings: PASS '+json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
