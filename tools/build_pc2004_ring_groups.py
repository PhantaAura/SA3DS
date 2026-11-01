#!/usr/bin/env python3
"""Add owner-only source ring-group definitions to existing direct-ring data.

Reuses the already converted shared ring model/texture. No owner model, SET row
or Ninja lookup table belongs in the redistributable code tree.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_emerald_coast_visual import read_source
HEADER=struct.Struct('<8sHHIIIII')
ROW=struct.Struct('<II32s')
SPEC=struct.Struct('<fffIIIII')
LUT_SHA='3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed'
FUNCTIONS={
 (0x79B2F0,0x293):'99ec4630cc1a2baa2e38dd489e200928d4680773af90dd6631c83f07689fd352',
 (0x79B140,0xC5):'29f6a77173c1b2d9c3c552497e3e86d57f4b4b779256a9278fb7f3b4158c023d',
 (0x79B210,0xDF):'84bd6f08408bf8aeb2ea14295ccb068aac11d0dcdae8224872f271d24fd53d76',
 (0x79B100,0x34):'067245896f4d310a9eeeb2de96021fa26020a74735fc5efa63a583487bdffe17',
}
def build_group_chunk(sources,load_squared,lut):
    if len(lut)!=4100 or hashlib.sha256(lut).hexdigest()!=LUT_SHA:raise ValueError('unsupported Ninja quarter-wave table')
    if len(sources)!=2 or {(s.level_id,s.act,s.variant) for s in sources}!={(1,0,'S'),(1,1,'S')}:
        raise ValueError('requires both normal Sonic S source banks')
    rows=[];counts=[0,0]
    for source in sorted(sources,key=lambda s:s.act):
        if (source.level_id,source.act) not in ((1,0),(1,1)):raise ValueError('unsupported source act')
        for index,p in enumerate(source.records):
            if p.type_word&0xfff!=21:continue
            if index>=1023:raise ValueError('group beyond original SET slot capacity')
            count,spacing,mode=p.parameters
            last=int(count);kind=int(mode)
            if count<0 or last>7 or kind not in (0,1):raise ValueError('group outside supported byte-journal line/circle subset')
            rows.append((source.act,index,p.raw))
            counts[source.act]+=last+1 if kind==0 else last
    if not rows or len(rows)>2046:raise ValueError('invalid bounded group count')
    offset=HEADER.size;spec=offset+len(rows)*ROW.size;trig=spec+SPEC.size
    data=bytearray(trig+len(lut))
    HEADER.pack_into(data,0,b'SA3DRGRP',1,0,len(rows),offset,spec,trig,len(data))
    for i,row in enumerate(rows):ROW.pack_into(data,offset+i*ROW.size,*row)
    SPEC.pack_into(data,spec,load_squared,load_squared+100,168100.,7,1025,0,0,0)
    data[trig:]=lut
    return bytes(data),counts,len(rows)
def build(exe,system,direct_package):
    read=verified_reader(exe)
    for (address,size),digest in FUNCTIONS.items():
        if hashlib.sha256(read(address,size)).hexdigest()!=digest:raise ValueError('ring-group function witness mismatch')
    registry=struct.unpack('<BBHfIII',read(0x1036420+21*20,20))
    if (registry[0],registry[1],registry[2],registry[4],registry[5])!=(2,3,1,0,0x79B2F0):raise ValueError('ring group registry mismatch')
    sources=[read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN')) for act in (0,1)]
    chunk,counts,groups=build_group_chunk(sources,registry[3],read(0x88AF78,4100))
    pak=sa3dspak.parse(direct_package)
    types=[c['type'] for c in pak['chunks']]
    if sorted(types)!=['METADATA','RINGS']:raise ValueError('expected original direct-ring placement package')
    metadata=json.loads(next(c['data'] for c in pak['chunks'] if c['type']=='METADATA'))
    metadata['ring_groups']={'source_exe_sha256':PROFILE_SHA256,'class':21,'groups':groups,
        'child_counts_by_act':counts,'source_set_sha256':[s.sha256 for s in sources],
        'quarter_lut_sha256':LUT_SHA,'scope':'line/circle, source byte journal, static owner definitions; not retail trace verified'}
    direct=next(c['data'] for c in pak['chunks'] if c['type']=='RINGS')
    if struct.unpack_from('<I',direct,16)[0]+sum(counts)>2046:raise ValueError('combined native ring actor capacity exceeded')
    chunks=[('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),
        ('RINGS',next(c['data'] for c in pak['chunks'] if c['type']=='RINGS'),0),('RING_GROUPS',chunk,0)]
    result=sa3dspak.build(chunks);sa3dspak.parse(result)
    return result,groups,counts
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('sonic_exe',type=Path);ap.add_argument('system',type=Path)
    ap.add_argument('direct_package',type=Path);ap.add_argument('output',type=Path)
    args=ap.parse_args()
    if args.output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):raise SystemExit('owner group output must remain outside the source tree')
    data,groups,counts=build(args.sonic_exe,args.system,args.direct_package.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    print(f'Owner ring groups: {groups} groups, children by act={counts}, bytes={len(data)}, SHA256={hashlib.sha256(data).hexdigest()}')
