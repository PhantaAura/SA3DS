from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
lab=(root/'runtime_0_0_9_0G/source/lab_gpu.c').read_text()
assert main.count('tick_input=sa3ds_view_input(tick_input);')==1
assert main.index('tick_input=sa3ds_ec_replay_input(')<main.index('tick_input=sa3ds_view_input(')<main.index('sa1_physics_step_steam_sonic_flat(')
assert 'sa3ds_view_model_yaw(g_player.yaw,g_camera.yaw)' in main
assert 'sa3ds_view_point((SA1Vec3){dx,dy,dz},camera_yaw)' in lab
assert 'cosf(-camera_yaw)' not in lab
assert 'sa3ds_ec_model_transform(&g_player,&actor);' in main
print('Native view bridge integration: PASS (both modes, replay, no duplicate input conversion)')
