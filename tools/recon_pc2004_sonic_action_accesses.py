#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

EXE_SIZE = 55549952
EXE_SHA256 = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
SONIC_ACTIONS_GLOBAL = 0x038F6E7C
SONIC_ACTION_COUNT = 149

REG_NAMES = ('eax','ecx','edx','ebx','esp','ebp','esi','edi')
OPS = {0x8B:'mov_load', 0x3B:'cmp_reg_mem', 0x39:'cmp_mem_reg'}

class ReconError(Exception): pass

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def text_file_offset_to_va(off: int) -> int:
    # Exact supported PC2004 executable: .text VA 0x00401000, raw offset 0x1000.
    if not (0x1000 <= off < 0x3DAF40):
        raise ReconError(f'offset outside exact .text mapping: {off:#x}')
    return 0x00400000 + off

def parse_immediate_table_access(data: bytes, address_pos: int):
    """Recognize: load [SONIC_ACTIONS_GLOBAL] into a GPR, then immediately
    access [that_reg + constant displacement]. This intentionally does not
    infer dataflow beyond the next instruction."""
    loaded_reg = None
    insn_start = None
    next_off = address_pos + 4

    # A1 moffs32 -> EAX
    if address_pos >= 1 and data[address_pos-1] == 0xA1:
        loaded_reg = 0
        insn_start = address_pos - 1
    # 8B /r with mod=00, r/m=101 -> absolute disp32 into reg field.
    elif address_pos >= 2 and data[address_pos-2] == 0x8B and (data[address_pos-1] & 0xC7) == 0x05:
        loaded_reg = (data[address_pos-1] >> 3) & 7
        insn_start = address_pos - 2
    else:
        return None

    if next_off + 2 > len(data): return None
    op = data[next_off]
    if op not in OPS: return None
    modrm = data[next_off+1]
    mod = (modrm >> 6) & 3
    rm = modrm & 7
    if rm != loaded_reg or rm == 4: return None  # SIB deliberately unsupported here.

    if mod == 0:
        if rm == 5: return None
        disp = 0
    elif mod == 1:
        if next_off + 3 > len(data): return None
        disp = struct.unpack_from('<b', data, next_off+2)[0]
    elif mod == 2:
        if next_off + 6 > len(data): return None
        disp = struct.unpack_from('<i', data, next_off+2)[0]
    else:
        return None

    if disp < 0 or disp % 4: return None
    index = disp // 4
    if index >= SONIC_ACTION_COUNT: return None
    return {
        'instruction_file_offset': insn_start,
        'instruction_va': text_file_offset_to_va(insn_start),
        'loaded_register': REG_NAMES[loaded_reg],
        'following_operation': OPS[op],
        'table_byte_offset': disp,
        'action_index': index,
    }

def analyze(path: Path):
    data = path.read_bytes()
    if len(data) != EXE_SIZE or sha256(data) != EXE_SHA256:
        raise ReconError('exact PC2004 sonic.exe fingerprint mismatch')
    pat = struct.pack('<I', SONIC_ACTIONS_GLOBAL)
    refs=[]; accesses=[]; pos=0
    while True:
        p=data.find(pat,pos)
        if p < 0: break
        refs.append(p)
        a=parse_immediate_table_access(data,p)
        if a: accesses.append(a)
        pos=p+1
    histogram={}
    for a in accesses:
        histogram[str(a['action_index'])]=histogram.get(str(a['action_index']),0)+1
    return {
        'file': {'size':len(data),'sha256':sha256(data)},
        'sonic_actions_global_va': SONIC_ACTIONS_GLOBAL,
        'known_action_count': SONIC_ACTION_COUNT,
        'raw_absolute_address_occurrences': len(refs),
        'immediate_constant_slot_accesses': len(accesses),
        'direct_action_indices_seen': sorted(int(k) for k in histogram),
        'access_histogram': histogram,
        'accesses': accesses,
        'evidence_scope': [
            'These are direct constant slot accesses immediately after loading ___SONIC_ACTIONS.',
            'They prove code references those table slots in this exact executable.',
            'They do NOT identify an action as idle/walk/run/jump/etc.',
            'Dynamic/indexed accesses are not decoded by this narrow scanner.'
        ]
    }

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('sonic_exe', type=Path)
    ap.add_argument('--json', type=Path)
    a=ap.parse_args()
    result=analyze(a.sonic_exe)
    text=json.dumps(result,indent=2)
    if a.json: a.json.write_text(text+'\n')
    print(text)

if __name__=='__main__': main()
