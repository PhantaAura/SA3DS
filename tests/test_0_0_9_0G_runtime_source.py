#!/usr/bin/env python3
from pathlib import Path

root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
mk=(root/'runtime_0_0_9_0G/Makefile').read_text()
lab=(root/'src/game/parity_lab.c').read_text()
trace=(root/'src/game/parity_trace.c').read_text()
hud=(root/'src/game/parity_hud.c').read_text()

required_main=[
    'sa3ds_parity_camera_toggle(&g_camera)',
    'sa3ds_parity_trace_write_frame',
    'PHYSICS_1TO1:NO',
    'memcmp(&physics_done,&g_player,sizeof(g_player))',
]
# Accept the original 0G banner or a later integrated White Zone revision.
assert ('SA3DS 0.0.9.0G - WHITE ZONE VALIDATION' in main or
        'SA3DS 0.0.9.1E-WIP7 - WHITE ZONE' in main)
for token in required_main: assert token in main, token
assert 'sa3ds_0_0_9_0G_white_zone_validation' in mk
assert 'SA3DS_PARITY_ZONE_TURN' in lab and 'AIR CONTROL / LANDING' in lab
assert 'gWorld.reset_position.z = zone->reset_z' in lab
for field in [
    'test_id','runtime_animation_id','action_slot','animation_frame','normal_x','slope_radians',
    'post_deadzone_x','filtered_input_magnitude','camera_yaw','local_forward_before',
    'accel_forward','turn_step','side_stopped_at_jog_threshold',
]:
    assert field in trace, field
assert 'SA3DS_PARITY_CAMERA_REFERENCE' in (root/'src/game/parity_hud.h').read_text()
assert 'camera->mode != SA3DS_PARITY_CAMERA_FOLLOW' in hud
print('0.0.9.0G 3DS runtime/source contract: PASS')
