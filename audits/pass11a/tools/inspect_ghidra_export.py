#!/usr/bin/env python3
"""Small read-only check of original 2004 PC Ghidra C export; not a decompiler."""
import argparse,re,sys
from pathlib import Path

def extract_function(code,addr):
    pat=re.compile(r'(?m)^\s*(?:void|int|float|float10|undefined4|uint|byte|bool|ulonglong|longlong)\s+(?:__\w+\s+)?FUN_'+addr+r'\s*\(')
    m=pat.search(code)
    if not m:raise ValueError('Missing Ghidra function FUN_'+addr)
    next_def=re.search(r'\n\s*(?:void|int|float|float10|undefined4|uint|byte|bool|ulonglong|longlong)\s+(?:__\w+\s+)?FUN_[0-9a-fA-F]{8}\s*\(',code[m.end():])
    end=m.end()+next_def.start() if next_def else min(len(code),m.end()+30000)
    return code[m.start():end]

def check(code):
    a=extract_function(code,'004050c0')
    b=extract_function(code,'00405260')
    c=extract_function(code,'004053d0')
    d=extract_function(code,'00407020')
    checks={
       'animated object vs static 0x40 branch':'0x40' in a and 'DAT_03ab9878' in a,
       'alternate rotation 0x20 flag':'0x20' in a and 'FUN_00781770' in a and 'FUN_007819c0' in a,
       'child link at object+0x2c':re.search(r'param_1\s*\+\s*0x2c',a) is not None,
       'sibling link at object+0x30':re.search(r'param_1\s*\+\s*0x30',a) is not None,
       'motion channels choose transform handlers':'0x20' in b and 'DAT_03ab98e8' in b,
       'ACTION evaluation traverses model':'FUN_004050c0' in c,
       'njAction delegates to action evaluator':'FUN_004053d0' in d,
    }
    for k,v in checks.items():print(('PASS' if v else 'FAIL')+' '+k)
    return all(checks.values())

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('sonic_exe_c');ar=ap.parse_args()
    try:
        txt=Path(ar.sonic_exe_c).read_text(encoding='utf-8',errors='replace')
        if not check(txt):sys.exit(1)
    except (OSError,ValueError) as e:print(f'FAIL: {e}',file=sys.stderr);sys.exit(1)
