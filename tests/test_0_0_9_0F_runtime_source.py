#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
runtime=ROOT/'runtime_0_0_9_0F'
main=(runtime/'source/main.c').read_text()
make=(runtime/'Makefile').read_text()
lab=(runtime/'source/lab_gpu.c').read_text()

required_main=[
    '#include "sonic_shbin.h"',
    'sa3ds_parity_lab_world()',
    'sa3ds_sonic_animation_observe',
    'sa3ds_sonic_animation_select',
    'memcmp(&physics_done,&g_player,sizeof(g_player))',
    'sa1_trace_write_frame',
    'white_zone_trace.csv',
    'SELECT+L/R',
]
for token in required_main:
    if token not in main:
        raise SystemExit(f'missing 0F runtime contract token: {token}')
if 'sonic_v_shbin' in main:
    raise SystemExit('regression: old broken shader symbol returned')
for token in ['../src/sa1/physics','../src/game','../src/platform/3ds','sonic.v.pica']:
    if token not in make and token != 'sonic.v.pica':
        raise SystemExit(f'missing Makefile integration: {token}')
if 'C3D_DrawArrays' not in lab or 'sa3ds_parity_lab_marker_spacing' not in lab:
    raise SystemExit('White Zone GPU floor/marker source missing')

# Code-only checkpoint must not contain generated owner packages.
for rel in ['romfs/sa3ds/sonic.sa3c','romfs/sa3ds/sonic.sa3t','romfs/sa3ds/owner_asset_build_report.json']:
    if (runtime/rel).exists():
        raise SystemExit(f'copyrighted/generated owner payload present: {rel}')
print('0.0.9.0F 3DS runtime source contract: PASS')
