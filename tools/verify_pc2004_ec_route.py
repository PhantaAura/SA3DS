#!/usr/bin/env python3
"""Exact-owner witnesses for Sonic's Emerald Coast act0->1 route, not gameplay parity."""
from pathlib import Path
import hashlib
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_collision_work import PROFILE_SHA256,parse_pe,va_to_off

SIGNATURES={
 'Emerald Coast level task':(0x90BF3C,'606d4f00'),
 'act0 controller state1':(0x4F6A7A,'c60601'),
 'act1 controller state2':(0x4F6AF7,'c60602'),
 'act2 controller state3':(0x4F6BA6,'c60603'),
 'state1 dispatch':(0x4F6D6F,'49742e'),
 'handoff center Z':(0x4F6CDF,'d90534af7e00'),
 'handoff center Y':(0x4F6CE9,'d90530af7e00'),
 'handoff center X':(0x4F6CF2,'d9052caf7e00'),
 'handoff distance and radius':(0x4F6D14,'e8d7042900d81d88097e00'),
 'strict handoff comparison':(0x4F6D24,'7a2d'),
 'advance by one act':(0x4F6D43,'6a01e836ecf1ff'),
 'move player to new start':(0x4F6D4A,'56e8c0daf1ff'),
 'Sonic start array selection':(0x41491D,'bfc8a59000'),
 'start row stride20':(0x414964,'83c714'),
 'movement Y argument':(0x43ECEA,'668b4e1866f7d90fb7d133c9e825613400'),
}

def verified_reader(path):
    image=Path(path).read_bytes()
    if hashlib.sha256(image).hexdigest()!=PROFILE_SHA256:raise ValueError('unsupported original executable identity')
    base,sections=parse_pe(image)
    def read(va,size):
        offset,_=va_to_off(va,base,sections)
        result=image[offset:offset+size]
        if len(result)!=size:raise ValueError('source read outside file')
        return result
    for label,(va,hexbytes) in SIGNATURES.items():
        expected=bytes.fromhex(hexbytes)
        if read(va,len(expected))!=expected:raise ValueError('instruction witness mismatch: '+label)
    return read

def route_rows(path):
    read=verified_reader(path)
    starts=[]
    for act in (0,1):
        level,source_act,x,y,z,yaw=struct.unpack('<HHfffI',read(0x90A5C8+act*20,20))
        if (level,source_act)!=(1,act) or yaw>65535:raise ValueError('unexpected Sonic start row')
        starts.append((act,0,x,y,z,yaw))
    center=struct.unpack('<fff',read(0x7EAF2C,12))
    radius=struct.unpack('<f',read(0x7E0988,4))[0]
    return starts,[(0,1,*center,radius)]

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_ec_route.py OWNER_SONIC_EXE')
    starts,edges=route_rows(sys.argv[1])
    print('PC2004 Emerald Coast Sonic route: PASS (exact source task dispatch, strict distance handoff and two original starts; no retail gameplay trace)')
