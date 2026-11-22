#!/usr/bin/env python3
"""Convert exact owner PC2004 automatic-loop paths to an external package."""
from pathlib import Path
import argparse
import json
import math
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader, PROFILE_SHA256

def build(exe):
    read=verified_reader(exe)
    entries=[]
    omitted=[]
    for i in range(22):
        levelact,table=struct.unpack('<II',read(0x91A858+i*8,8))
        if levelact not in (0x100,0x101):continue
        for j in range(64):
            head,=struct.unpack('<I',read(table+4*j,4))
            if not head:break
            wrap,_,count,length,points,callback=struct.unpack('<BBHfII',read(head,16))
            if callback!=0x4BB1F0:
                omitted.append({'act':levelact&255,'ordinal':j,'callback':hex(callback)})
                continue
            if wrap>1 or not 2<=count<=512 or not math.isfinite(length) or length<=0:
                raise ValueError('invalid source loop header')
            rows=[]
            for k in range(count):
                _,_,segment,x,y,z=struct.unpack('<hhffff',read(points+20*k,20))
                if not all(math.isfinite(v) for v in (segment,x,y,z)) or segment<0:
                    raise ValueError('invalid source loop point')
                rows.append(struct.pack('<ffff',segment,x,y,z))
            entries.append(struct.pack('<IIIIf',levelact&255,j,wrap,count,length)+b''.join(rows))
        else:raise ValueError('unterminated source path list')
    if not entries:raise ValueError('no automatic loop paths')
    data=struct.pack('<8sII',b'SA3DLOOP',1,len(entries))+b''.join(entries)
    meta={'source_sha256':PROFILE_SHA256,'supported_callback':'4BB1F0',
          'omitted_other_path_callbacks':omitted,'status':'CANDIDATE; not full stage path support'}
    return sa3dspak.build([('LOOP_PATHS',data,0),('METADATA',json.dumps(meta,sort_keys=True).encode(),0)])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('sonic_exe',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args()
    if a.output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        p.error('owner paths must remain outside the code tree')
    result=build(a.sonic_exe)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(result)
if __name__=='__main__':main()
