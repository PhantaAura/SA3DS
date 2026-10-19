#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from recon_pc2004_sonic_action_accesses import analyze

exe=Path('/mnt/data/sonic.exe')
if not exe.exists():
    print('SKIP exact /mnt/data/sonic.exe not mounted')
    raise SystemExit(0)
r=analyze(exe)
assert r['raw_absolute_address_occurrences']==193
assert r['immediate_constant_slot_accesses']==190
assert r['direct_action_indices_seen']==[0,1,2,3,4,5,6,7,20,77,100,122,123]
assert r['access_histogram']['1']==76
assert r['access_histogram']['123']==41
print('PC2004 Sonic ACTION direct-slot access atlas: PASS')
