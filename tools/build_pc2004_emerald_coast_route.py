#!/usr/bin/env python3
"""Build external owner-only route metadata from the exact PC2004 executable."""
from pathlib import Path
import argparse
import json
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import route_rows,PROFILE_SHA256
HEADER=struct.Struct('<8sHHIIIII')
START=struct.Struct('<IIfffI')
EDGE=struct.Struct('<IIffff')

def build(starts,edges):
    start_offset=HEADER.size;edge_offset=start_offset+len(starts)*START.size
    data=bytearray(edge_offset+len(edges)*EDGE.size)
    HEADER.pack_into(data,0,b'SA3DECRO',1,0,1,len(starts),len(edges),start_offset,edge_offset)
    for i,row in enumerate(starts):START.pack_into(data,start_offset+i*START.size,*row)
    for i,row in enumerate(edges):EDGE.pack_into(data,edge_offset+i*EDGE.size,*row)
    metadata=json.dumps({'source_profile':'SADX_PC_2004_CANONICAL','source_sha256':PROFILE_SHA256,
        'character':'Sonic','stage':1,'required_acts':[0,1],
        'authority':'EXACT-EXECUTABLE STATIC EVIDENCE; NOT RETAIL TRACE VERIFIED'},sort_keys=True).encode()
    return sa3dspak.build([('METADATA',metadata,0),('ROUTE',bytes(data),0)])

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    if args.output.resolve().is_relative_to(root):raise SystemExit('owner-generated route data must stay outside the code tree')
    starts,edges=route_rows(args.sonic_exe)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(build(starts,edges))
    print(f'Owner route package: {len(starts)} source starts,{len(edges)} source act handoff; {args.output}')
if __name__=='__main__':main()
