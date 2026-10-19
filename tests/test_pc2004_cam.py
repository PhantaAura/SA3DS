#!/usr/bin/env python3
"""0.0.9.1D source-independent adversarial CAM tests plus owner-byte checks."""
from __future__ import annotations
import hashlib
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_cam import (
    read_cam_source,list_cam_sources,parse_record,build_cam_chunk,parse_cam_chunk,
    CamDecodeError,camera_group,HEADER,DIRECTORY,
)
from tools.build_pc2004_cam_packages import build_cam_packages
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.build_pc2004_emerald_coast_visual import build_act_package,read_source
from tools import sa3dspak

def raises(fn):
    try: fn()
    except (ValueError,UnicodeError,struct.error):return
    raise AssertionError('expected decoder to reject malformed input')

def fixture():
    row1=bytearray(64)
    struct.pack_into('<HHI',row1,0,39,512,0x40000000)
    struct.pack_into('<I',row1,32,0xffd00000) # exact sentinel from owner files
    row2=bytearray(64)
    struct.pack_into('<HHI',row2,0,31,533,0x12)
    struct.pack_into('<fff',row2,8,100.0,-20.0,30.0)
    return struct.pack('<I',2)+bytes(60)+bytes(row1)+bytes(row2)

def test_synthetic():
    raw=fixture();src=read_cam_source('CAM0100S.BIN',raw)
    assert src.source_bytes()==raw and src.records[0].word_00==39
    assert src.records[0].word_02==512 and src.records[0].word_04==0x40000000
    assert src.records[0].raw[32:36]==bytes.fromhex('0000d0ff')
    neutral=build_cam_chunk([src]); info=parse_cam_chunk(neutral)
    assert (info['files'],info['records'])==(1,2)
    assert neutral==build_cam_chunk([src])
    empty=read_cam_source('CAMMI0402S.BIN',struct.pack('<I',0)+bytes(60))
    assert parse_cam_chunk(build_cam_chunk([empty]))['records']==0
    assert camera_group('CAM0102B.BIN')==(1,2,'B')
    assert camera_group('CAMSS00S.BIN') is None
    raises(lambda:read_cam_source('CAM0100S.DAT',raw))
    raises(lambda:read_cam_source('CAM0100S.BIN',raw[:17]))
    raises(lambda:read_cam_source('CAM0100S.BIN',raw[:-1]))
    raises(lambda:read_cam_source('CAM0100S.BIN',struct.pack('<I',999999)+raw[4:]))
    raises(lambda:read_cam_source('CAM0100S.BIN',raw[:4]+b'\x01'+raw[5:]))
    raises(lambda:build_cam_chunk([src,src]))
    for off in [0,8,HEADER.size-1,len(neutral)-1]:
        corrupt=bytearray(neutral);corrupt[off]^=0x20
        raises(lambda c=corrupt:parse_cam_chunk(bytes(c)))
    corrupt=bytearray(neutral)
    directory_offset=((HEADER.size+15)//16)*16
    corrupt[directory_offset+32+4+4]^=1
    raises(lambda:parse_cam_chunk(bytes(corrupt)))
    corrupt=bytearray(neutral)
    _,_,_,_,_,_,_,rows_off,_,_=HEADER.unpack_from(neutral)
    corrupt[rows_off+4]^=1
    raises(lambda:parse_cam_chunk(bytes(corrupt)))
    with TemporaryDirectory() as d:
        zpath=Path(d)/'dup.zip'
        with ZipFile(zpath,'w') as z:
            z.writestr('system/CAM0100S.BIN',raw)
            z.writestr('mods/CAM0100S.BIN',raw)
        raises(lambda:list_cam_sources(zpath))
    print('0.0.9.1D CAM synthetic/NaN/CRC/SHA/duplicate tests: PASS')

def test_owner(system:Path,exe:Path|None):
    sources=list_cam_sources(system)
    n=sum(len(s.records) for s in sources)
    assert len(sources)==127,(len(sources),n)
    assert n==4955,n
    assert len([s for s in sources if not s.records])==2
    assert all(hashlib.sha256(s.source_bytes()).hexdigest()==s.sha256 for s in sources)
    ec={
        'CAM0100S.BIN','CAM0100E.BIN','CAM0101S.BIN','CAM0102S.BIN','CAM0102B.BIN'
    }
    ec_sources=[s for s in sources if s.name in ec]
    assert len(ec_sources)==5
    for s in ec_sources:
        assert read_source(system,s.name)==s.source_bytes()
    with TemporaryDirectory() as d:
        report=build_cam_packages(system,Path(d)/'all')
        assert report['total_files']==127 and report['total_records']==4955
        assert report['total_groups']>=20
        for entry in report['groups']:
            pak=sa3dspak.parse((Path(d)/'all'/entry['output']).read_bytes())
            assert [c['type'] for c in pak['chunks']]==['METADATA','CAMERA']
            assert parse_cam_chunk(pak['chunks'][1]['data'])['records']==entry['records']
        ec_report=build_cam_packages(system,Path(d)/'ec',level_id=1)
        assert ec_report['total_files']==5
        assert len(ec_report['groups'])==3
        if exe:
            dec=PC2004StageDecoder(exe)
            for act in (0,1,2):
                table=dec.landtable(1,act)
                pvm=read_source(system,table.texture_archive+'.PVM')
                cam=[s for s in ec_sources if camera_group(s.name)[1]==act]
                pak,metadata=build_act_package(dec,act,pvm,cam_sources=cam)
                parsed=sa3dspak.parse(pak)
                assert [c['type'] for c in parsed['chunks']]==[
                    'METADATA','GEOMETRY','MATERIALS','TEXTURES','CAMERA']
                assert metadata['camera_setup']['camera_behavior_implemented'] is False
                assert parse_cam_chunk(parsed['chunks'][-1]['data'])['records']==sum(len(x.records) for x in cam)
    print(f'0.0.9.1D CAM owner-byte tests: {len(sources)} files / {n} records / 5 EC files PASS')

def main():
    test_synthetic()
    if len(sys.argv)>=2:
        test_owner(Path(sys.argv[1]),Path(sys.argv[2]) if len(sys.argv)>2 else None)
if __name__=='__main__': main()
