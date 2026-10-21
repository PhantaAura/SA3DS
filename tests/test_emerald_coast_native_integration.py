from pathlib import Path

root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
gpu=(root/'runtime_0_0_9_0G/source/emerald_coast_gpu.c').read_text()
sonic=(root/'runtime_0_0_9_0G/source/sonic_gpu.c').read_text()
mk=(root/'runtime_0_0_9_0G/Makefile').read_text()

assert 'if (down&KEY_R) return 6;' in main
assert 'sdmc:/3ds/SA3DS/assets/emerald_coast_act0.sa3dspak' in main
assert 'sa3ds_ec_visual_open(&g_ec_visual' in main
assert 'sa3ds_ec_gpu_init(&g_ec_gpu,&g_ec_visual,500.0f' in main
assert 'return sa3ds_ec_gpu_draw(&g_ec_gpu);' in main
assert 'g_pose_lab || g_emerald_coast || g_paused' in main
assert 'sa3ds_upload_rgba8_cpu' in gpu and 'C3D_SyncDisplayTransfer' not in gpu
assert 'g->resident_texture_count++' in gpu
assert 'sa3dspak' in mk
assert 'sa3ds_upload_rgba8_cpu' in sonic
print('Emerald Coast native stage integration: PASS (external SD, bounded subset, CPU texture upload)')
