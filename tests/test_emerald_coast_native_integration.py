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
assert 'if (!sa3ds_ec_gpu_draw(&g_ec_gpu)) return 0;' in main
assert 'if (!g_ec_combined) return 1;' in main
assert 'g_pose_lab || (g_emerald_coast&&!g_ec_combined) || g_paused' in main
assert 'sa3ds_ec_collision_resolve_walls(&g_ec_collision,previous,&g_player)' in main
assert 'g_emerald_coast?&g_ec_collision.world:sa3ds_parity_lab_world()' in main
assert 'sonic_mv=g_ec_view;' in main
boot_menu=main[main.index('static int boot_menu(void)'):main.index('static void boot_result')]
assert 'gfxSwapBuffers();' in boot_menu
stage_model=main[main.index('if(g_emerald_coast){\n        sonic_mv=g_ec_view;'):main.index('} else if (g_pose_lab)',main.index('sonic_mv=g_ec_view;'))]
assert 'Mtx_Scale' not in stage_model
assert 'sa3ds_upload_rgba8_cpu' in gpu and 'C3D_SyncDisplayTransfer' not in gpu
assert 'g->resident_texture_count++' in gpu
assert 'return sa3ds_ec_instance_is_visible(x) &&' in gpu
assert 'sa3dspak' in mk
assert 'sa3ds_upload_rgba8_cpu' in sonic
print('Emerald Coast native stage integration: PASS (external SD, bounded subset, CPU texture upload)')
