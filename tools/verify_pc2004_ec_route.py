#!/usr/bin/env python3
"""Exact-owner witnesses for Sonic's Emerald Coast act0->1 route, not gameplay parity."""
from pathlib import Path
import hashlib
import math
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_collision_work import PROFILE_SHA256,parse_pe,va_to_off
from tools.importers.pc2004_stage import PC2004StageDecoder

SIGNATURES={
 'Emerald Coast level task':(0x90BF3C,'606d4f00'),
 'act0 controller state1':(0x4F6A7A,'c60601'),
 'act1 controller state2':(0x4F6AF7,'c60602'),
 'act2 controller state3':(0x4F6BA6,'c60603'),
 'state1 dispatch':(0x4F6D6F,'49742e'),
 'handoff center Z':(0x4F6CDF,'d90534af7e00'),
 'handoff center Y':(0x4F6CE9,'d90530af7e00'),
 'handoff center X':(0x4F6CF2,'d9052caf7e00'),
 'handoff squared distance float store':(0x4F6D0B,'d91c24ddd8ddd8ddd8'),
 'handoff distance and radius':(0x4F6D14,'e8d7042900d81d88097e00'),
 'strict handoff comparison':(0x4F6D24,'7a2d'),
 'advance by one act':(0x4F6D43,'6a01e836ecf1ff'),
 'move player to new start':(0x4F6D4A,'56e8c0daf1ff'),
 'Sonic start array selection':(0x41491D,'bfc8a59000'),
 'start row stride20':(0x414964,'83c714'),
 'movement Y argument':(0x43ECEA,'668b4e1866f7d90fb7d133c9e825613400'),
 'death check stage task registration':(0x415A2A,'6880ae44006a016a02e8285effff'),
 'death check ticks existing children first':(0x44AE90,'e88b05fcff'),
 'death check task gate':(0x44AEA0,'8b430c85c00f855c0100008b4320f64004040f854f010000'),
 'death check mesh projection':(0x44AF50,'e8bbb5000083c40885c07433'),
 'death check upper and lower results':(0x44AF5C,'8b44241c85c0752b8b44243c85c07423'),
 'death check distance and call':(0x44AF6C,'d9442414d864244cd9e1d81dc8f17d00dfe0f6c4417a0c6a008bc3e864bbffff'),
 'death check distance limit':(0x7DF1C8,'0000f041'),
 'death task constructor invokes source executor':(0x446AF0,'5068800c44006a08e8434efcff'),
 'death task Sonic dispatch entry':(0x446AA8,'036a4400'),
 'fall-death sound':(0x446A21,'68df050000e845d3fdff'),
 'death-zone level table':(0x4751CD,'8b049508599100'),
 'death-zone act lookup':(0x4751E7,'8b3488'),
 'death-zone character mask filter':(0x475225,'853e7412'),
 'death-zone model projection and match branch':(0x47522F,'e8dc12feff83c40885c07515'),
 'death-zone end-of-list result':(0x475245,'5eb801000000'),
 'death-zone match result':(0x475250,'5e33c0'),
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

def verify_death_surfaces(path):
    decoder = PC2004StageDecoder(Path(path))
    for act, expected_count in enumerate((1, 2, 1)):
        surfaces = decoder.emerald_coast_death_surfaces(act)
        assert len(surfaces) == expected_count
        for surface in surfaces:
            assert surface.character_mask == 0xff
            assert len(surface.points) == 4 and len(surface.triangles) == 2
            assert {index for tri in surface.triangles for index in tri} == set(range(4))
            assert all(math.isfinite(value) for point in surface.points for value in point)
            assert len({point[1] for point in surface.points}) == 1
            for a, b, c in surface.triangles:
                ax, _, az = surface.points[a]
                bx, _, bz = surface.points[b]
                cx, _, cz = surface.points[c]
                assert (bx-ax)*(cz-az) != (bz-az)*(cx-ax)
        if act == 1:
            assert surfaces[0].points[0][1] < 0 < surfaces[1].points[0][1]
    print('PC2004 Emerald Coast death zones: PASS (four finite quads, character masks, transforms, and stage-task geometry gate; native dispatch pending)')


if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_ec_route.py OWNER_SONIC_EXE')
    starts,edges=route_rows(sys.argv[1])
    print('PC2004 Emerald Coast Sonic route: PASS (exact source task dispatch, strict distance handoff and two original starts; no retail gameplay trace)')
    verify_death_surfaces(sys.argv[1])
