#!/usr/bin/env python3
"""Source contract: visual preflight must guard actual Old-3DS upload path."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
gpu = (root / 'runtime_0_0_9_0F/source/sonic_gpu.c').read_text()
main = (root / 'runtime_0_0_9_0F/source/main.c').read_text()
make = (root / 'runtime_0_0_9_0F/Makefile').read_text()
for token in ('#include "sa3c_gpu_preflight.h"', 'sa3ds_visual_preflight(c,t,&visual)',
              'sa3ds_visual_material_rgba(rd32(m))', 'C3D_SyncDisplayTransfer'):
    assert token in gpu, f'missing GPU visual contract: {token}'
for token in ('sa3ds_visual_preflight(&g_character,&g_texture_archive,&g_visual_report)',
              'sa3ds_visual_result_name(visual)', 'g_visual_report.partially_transparent_texels'):
    assert token in main, f'missing runtime visual diagnostics: {token}'
assert '../src/sa3c' in make, 'preflight source missing from native build'
assert 'if (r==0 && g==0 && b==0) r=g=b=255' not in gpu, 'black diffuse clobbered'
assert 'sa3ds_visual_material_rgba' in (root / 'src/sa3c/sa3c_gpu_preflight.c').read_text()
print('White Zone native visual integration/source contract: PASS')
