from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
assert "g_wca_replay=g_ec_replay && replay_mode=='M';" in main
assert 'if(wca.reset)reset_selected_zone();' in main
assert 'tick_input=wca.screen_input;' in main
assert 'g_camera.yaw=wca.camera_yaw;' in main
assert 'SA3DS_WCA_REPLAY_TICKS:600u' in main
assert 'g_wca_replay?replay_name:' in main
assert '(g_ec_replay || g_ring_probe || g_checkpoint_probe || (!modifier && (held&(KEY_L|KEY_R))))' in main
assert main.index('tick_input=wca.screen_input;')<main.index('tick_input=sa3ds_view_input(tick_input);')<main.index('sa1_physics_step_steam_sonic_flat(&g_player,&tick_input')
assert main.count('tick_input=sa3ds_view_input(tick_input);')==1
print('WCA native replay integration: PASS (opt-in, fixed camera, real controller, per-case trace labels)')
