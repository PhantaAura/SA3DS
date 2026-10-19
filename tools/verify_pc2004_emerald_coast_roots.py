#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import argparse, json, struct, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder, LANDTABLE_TABLE_VA
from tools.importers.pc2004_chrmodels import DecodeError

CHECKS = {
    0x0043A26C: bytes.fromhex('8b349508da9700'),  # mov esi,[edx*4+0x97da08]
    0x0043A27B: bytes.fromhex('0fbf06'),          # movsx eax,word [esi]
    0x0043A280: bytes.fromhex('8b4e0c'),          # mov ecx,[esi+0x0c]
    0x0043A287: bytes.fromhex('8d7918'),          # lea edi,[ecx+0x18]
    0x0043A290: bytes.fromhex('8b4708'),          # flags at COL +0x20 via edi+8
    0x0043A2AA: bytes.fromhex('83c724'),          # COL stride 0x24
    0x00422B5E: bytes.fromhex('68d0e47d00'),      # push "_Stg01.rel"
    0x0051F680: bytes.fromhex('c3'),              # static-link rel-loader stub
}

def main(argv=None):
    ap=argparse.ArgumentParser(); ap.add_argument('exe',type=Path); ap.add_argument('--json',type=Path)
    args=ap.parse_args(argv)
    try:
        dec=PC2004StageDecoder(args.exe)
        for va,expected in CHECKS.items():
            got=dec.pe.read_bytes(va,len(expected))
            if got!=expected: raise DecodeError(f'code witness mismatch at {va:#x}: {got.hex()} != {expected.hex()}')
        name=dec._cstring_va(0x007DE4D0,32)
        if name!='_Stg01.rel': raise DecodeError('Emerald Coast rel marker mismatch')
        acts=dec.emerald_coast_landtables()
        expected_roots=(0x00E99CB8,0x01039E9C,0x010386B4)
        expected_counts=(480,169,199)
        expected_tex=('BEACH01','BEACH02','BEACH03')
        rows=[]
        for a,root,count,tex in zip(acts,expected_roots,expected_counts,expected_tex):
            if a.va!=root or a.collision_count!=count or a.texture_archive!=tex:
                raise DecodeError(f'EC act {a.act} root/count/archive mismatch')
            if a.collision_list_va + a.collision_count*0x24 != a.va:
                raise DecodeError(f'EC act {a.act} collision list does not end exactly at LandTable')
            rows.append({'act':a.act,'landtable_va':f'0x{a.va:08x}','collision_count':a.collision_count,
                         'collision_list_va':f'0x{a.collision_list_va:08x}','texture_archive':a.texture_archive,
                         'texture_list_va':f'0x{a.texture_list_va:08x}'})
        report={'status':'PASS','source_sha256':dec.sha256,'landtable_table_va':f'0x{LANDTABLE_TABLE_VA:08x}',
                'stage_rel_marker':name,'stage_rel_loader_0x51f680':'RET/static-link stub','acts':rows}
        text=json.dumps(report,indent=2,sort_keys=True)
        if args.json: args.json.write_text(text+'\n')
        print(text); return 0
    except (OSError,DecodeError,ValueError) as e:
        print('error:',e,file=sys.stderr); return 1
if __name__=='__main__': raise SystemExit(main())
