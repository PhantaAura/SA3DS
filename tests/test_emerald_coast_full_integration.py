"""Native wiring only; actual full-act loading requires owner assets/emulator."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
assert 'g_ec_combined&&!g_ec_replay&&sa3ds_read_file(' in main
assert 'emerald_coast_route.sa3dspak' in main
assert 'sa3ds_ec_gpu_init_full(&g_ec_gpu,&g_ec_visual' in main
assert 'sa3ds_ec_gpu_init(&g_ec_gpu,&g_ec_visual,500.0f' in main
assert 'sa3ds_ec_route_next(&g_ec_route,g_ec_act_index,sa3ds_ec_task_origin(&g_player)' in main
assert 'sa3ds_ec_residency_recenter(&g_ec_collision,g_ec_collision_bytes' in main
assert 'g_ec_gpu.visual=&g_ec_visual;' in main
assert 'g_ec_collision.world.context=&g_ec_collision;' in main
handoff=main[main.index('static int ec_change_act('):main.index('static void ec_refresh_collision_region(')]
assert handoff.index('C3D_FrameSync();')<handoff.index('sa3ds_ec_gpu_destroy(&g_ec_gpu)')
assert handoff.index('sa3ds_ec_gpu_init_full(&gpu')<handoff.index('sa3ds_ec_gpu_destroy(&g_ec_gpu)')
assert 'sa3ds_ec_gpu_destroy(&gpu);sa3ds_ec_collision_destroy(&collision);sa3ds_free_file(bytes)' in handoff
assert 'sa3ds_free_file(g_ec_collision_bytes);' in main
assert 'not natural stage progression' in main
print('Emerald Coast full-act wiring: PASS (source route, transactional act replacement, stable ownership, bounded support regions; native gameplay gate separate)')
