#!/usr/bin/env python3
"""Build external owner-only route metadata from the exact PC2004 executable."""
from pathlib import Path
import argparse
import json
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import route_rows, verified_reader, PROFILE_SHA256
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.importers.pc2004_set import read_set_source
HEADER=struct.Struct('<8sHHIIIII')
START=struct.Struct('<IIfffI')
EDGE=struct.Struct('<IIffff')
GOAL=struct.Struct('<IIIfffff')

def goal_row(exe, system):
    read = verified_reader(exe)
    kind = 81
    flags, task_list, clip, load_distance, _, main, name = struct.unpack(
        '<BBHfIII', read(0x1036420 + kind * 20, 20))
    if (flags, task_list, clip, main, read(name, 5)) != (2, 3, 1, 0x4F9FB0, b'O BZ '):
        raise ValueError('unexpected O BZ registry')
    witnesses = {
        0x4F9FDA: 'c60602',         # initialize to action 2
        0x4FA00C: 'e89f840b00',     # action 2 calls the ordinary goal helper
        0x5B24CB: 'e870f3e8ff',     # CCL center check
        0x5B24D3: '83f8017508',     # only player 0 ordinal
        0x5B24D8: 'e86330e6ffc60605', # results request then action 5
        0x415789: '68f04141006a036a00', # ordinary results task
        0x414220: 'f6460403',       # wait for either ground bit
    }
    for address, expected in witnesses.items():
        if read(address, len(bytes.fromhex(expected))) != bytes.fromhex(expected):
            raise ValueError(f'goal instruction witness mismatch at {address:#x}')
    filename = 'SET0101S.BIN'
    source = read_set_source(filename, read_source(system, filename))
    goals = [(i, row) for i, row in enumerate(source.records) if row.type_word & 4095 == kind]
    if len(goals) != 1 or goals[0][0] >= 1023:
        raise ValueError('unexpected Sonic finish placement')
    index, row = goals[0]
    add = struct.unpack('<f', read(0x7DCB10, 4))[0]
    scale = struct.unpack('<f', read(0x7E07E4, 4))[0]
    return (1, index, row.type_word, *row.position,
            (row.parameters[0] + add) * scale, load_distance)

def build(starts,edges,goal=None):
    start_offset=HEADER.size;edge_offset=start_offset+len(starts)*START.size
    end=edge_offset+len(edges)*EDGE.size
    data=bytearray(end+(GOAL.size if goal else 0))
    HEADER.pack_into(data,0,b'SA3DECRO',1,int(goal is not None),1,len(starts),len(edges),start_offset,edge_offset)
    if goal:GOAL.pack_into(data,end,*goal)
    for i,row in enumerate(starts):START.pack_into(data,start_offset+i*START.size,*row)
    for i,row in enumerate(edges):EDGE.pack_into(data,edge_offset+i*EDGE.size,*row)
    metadata=json.dumps({'source_profile':'SADX_PC_2004_CANONICAL','source_sha256':PROFILE_SHA256,
        'character':'Sonic','stage':1,'required_acts':[0,1],
        'authority':'EXACT-EXECUTABLE STATIC EVIDENCE; NOT RETAIL TRACE VERIFIED'},sort_keys=True).encode()
    return sa3dspak.build([('METADATA',metadata,0),('ROUTE',bytes(data),0)])

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe',type=Path);parser.add_argument('output',type=Path)
    parser.add_argument('--system',type=Path,help='owner system directory (default: sibling of sonic.exe)')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    if args.output.resolve().is_relative_to(root):raise SystemExit('owner-generated route data must stay outside the code tree')
    starts,edges=route_rows(args.sonic_exe)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    goal=goal_row(args.sonic_exe,args.system or args.sonic_exe.parent/'system')
    args.output.write_bytes(build(starts,edges,goal))
    print(f'Owner route package: {len(starts)} source starts,{len(edges)} source act handoff, O BZ finish row {goal[1]}; {args.output}')
if __name__=='__main__':main()
