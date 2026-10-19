#!/usr/bin/env python3
from pathlib import Path
import json,re,sys
ROOT=Path(__file__).resolve().parents[1]
rt=ROOT/'runtime_0_0_9_0E'
main=(rt/'source/main.c').read_text()
gpu=(rt/'source/sonic_gpu.c').read_text()
mk=(rt/'Makefile').read_text()
shader=(rt/'source/sonic.v.pica').read_text()
prep=(ROOT/'tools/prepare_3ds_owner_assets.py').read_text()

req_main=['C3D_Init','C3D_RenderTargetCreate','C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL)','C3D_AlphaTest(true,GPU_GREATER,15)','sa3c_pose_sample_action','sa3c_pose_build_world','sa3ds_sonic_gpu_draw']
for s in req_main:
    assert s in main,s
req_gpu=['C3D_TexInit','C3D_SyncDisplayTransfer','GX_TRANSFER_OUT_TILED(1)','__builtin_bswap32','C3D_DrawArrays(GPU_TRIANGLES','sa3c_render_emit']
for s in req_gpu: assert s in gpu,s
assert '-lcitro3d -lctru -lm' in mk
assert 'ROMFS       := romfs' in mk
assert 'sonic_shbin.h' in main
assert 'sonic_v_shbin.h' not in main
assert 'sonic_shbin_size' in main
assert '.fvec projection[4], modelView[4]' in shader
assert 'CHRMODELS_SHA="76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5"' in prep
assert 'SONIC_PVM_SHA="f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe"' in prep
expected={'Stand':1,'Walk1':3,'Run1':6,'JumpOrSpin':14,'Roll':14,'Fall':10,'Land':11,'Brake':7}
for name,slot in expected.items():
    assert re.search(r'\{"%s"\s*,\s*\d+\s*,\s*%d\}'%(re.escape(name),slot),main),(name,slot)
# Checkpoint must not ship generated source-derived payloads.
for p in rt.rglob('*'):
    if p.is_file():
        assert p.suffix.lower() not in {'.sa3c','.sa3t'},f'forbidden generated payload: {p}'
print('0.0.9.0E Citro3D runtime source contract: PASS')
