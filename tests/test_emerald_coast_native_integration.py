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
assert 'if (!sa3ds_ec_gpu_draw(&g_ec_gpu,0)) return 0;' in main
assert 'if (!g_ec_combined) return sa3ds_ec_gpu_draw(&g_ec_gpu,1);' in main
assert 'g_pose_lab || (g_emerald_coast&&!g_ec_combined) || g_paused' in main
assert 'sa3ds_ec_collision_resolve_walls(&g_ec_collision,previous,&g_player)' in main
assert 'g_emerald_coast?&g_ec_collision.world:sa3ds_parity_lab_world()' in main
assert 'sa3ds_ec_model_transform(&g_player,&actor);' in main
assert 'Mtx_Multiply(&sonic_mv,&g_ec_view,&model);' in main
assert 'g_ec_model_lift' not in main
boot_menu=main[main.index('static int boot_menu(void)'):main.index('static void boot_result')]
assert 'gfxSwapBuffers();' in boot_menu
stage_model=main[main.index('SA3CMatrix actor;'):main.index('} else if (g_pose_lab)',main.index('SA3CMatrix actor;'))]
assert 'Mtx_Scale' not in stage_model
assert main.index('sa3ds_ec_gpu_draw(&g_ec_gpu,0)')<main.index('sa3ds_sonic_gpu_draw(&g_sonic_gpu')<main.rindex('sa3ds_ec_gpu_draw(&g_ec_gpu,1)')
assert 'sa3ds_upload_rgba8_cpu' in gpu and 'C3D_SyncDisplayTransfer' not in gpu
assert 'g->resident_texture_count++' in gpu
assert 'return sa3ds_ec_instance_is_visible(x) &&' in gpu
assert gpu.count('bucket=mesh.material_index+water*v->material_count')==2
assert 'd->deferred_water!=(uint8_t)deferred_water' in gpu
assert 'deferred_water?GPU_WRITE_COLOR:GPU_WRITE_ALL' in gpu
assert 'C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);' in gpu
assert 'sa3dspak' in mk
assert 'sa3ds_upload_rgba8_cpu' in sonic
print('Emerald Coast native stage integration: PASS (external SD, bounded subset, CPU texture upload)')
