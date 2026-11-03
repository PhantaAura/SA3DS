#!/usr/bin/env python3
"""Guard that the actual Citro3D source uses per-draw vertex arena spans."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
gpu=(root/'runtime_0_0_9_0F/source/sonic_gpu.c').read_text()
hdr=(root/'runtime_0_0_9_0F/include/sonic_gpu.h').read_text()
make=(root/'runtime_0_0_9_0F/Makefile').read_text()
assert 'sa3ds_gpu_batch_budget(c,&r->batch)' in gpu
assert 'sa3ds_gpu_batch_reserve(&r->batch,cmd->index_count,&first_vertex)' in gpu
assert 'SA3DSGpuVertex *dst=r->scratch+first_vertex' in gpu
assert 'BufInfo_Add(buf,dst,sizeof(*dst),2,0x10)' in gpu
assert 'GSPGPU_FlushDataCache(dst,' in gpu
assert 'sa3ds_gpu_batch_reset(&r->batch)' in gpu
assert 'SA3DSGpuBatch batch;' in hdr
lab=(root/'runtime_0_0_9_0F/source/lab_gpu.c').read_text()
assert 'SA3DSLabVertex *zone_vertices = &lab->scratch[cursor]' in lab
assert 'uint32_t marker_start = cursor' in lab
assert 'write_quad(&lab->scratch[marker_start+n]' in lab
assert 'bind_vertices(markers)' in lab
assert 'bind_vertices(lab->scratch)' not in lab
main=(root/'runtime_0_0_9_0F/source/main.c').read_text()
assert 'sa3ds_gpu_batch_result_name(g_sonic_gpu.last_batch_error)' in main
assert '../src/sa3c' in make
assert 'r->scratch_capacity=max_mesh_indices' not in gpu
print('White Zone Citro3D per-draw vertex isolation contract: PASS')

# The active0G runtime must also preserve queued vertices across actor draws.
gpu=(root/'runtime_0_0_9_0G/source/sonic_gpu.c').read_text()
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
assert 'sa3ds_gpu_batch_budget_instances(c,instances,&r->batch)' in gpu
begin=gpu.split('void sa3ds_sonic_gpu_begin_frame(',1)[1].split('int sa3ds_sonic_gpu_draw_append(',1)[0]
append=gpu.split('int sa3ds_sonic_gpu_draw_append(',1)[1].split('int sa3ds_sonic_gpu_draw(',1)[0]
assert 'sa3ds_gpu_batch_reset' in begin and 'sa3ds_gpu_batch_reset' not in append
assert 'sa3ds_sonic_gpu_begin_frame(&g_saru_gpu)' in main
assert 'sa3ds_sonic_gpu_draw_append(&g_saru_gpu' in main
print('Current0G shared actor arena reset/append contract: PASS')
