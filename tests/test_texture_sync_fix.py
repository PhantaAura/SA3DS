#!/usr/bin/env python3
"""Guard against the hardware-observed texture-transfer deadlock regression."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]
s=(p/'runtime_0_0_9_0G/source/sonic_gpu.c').read_text()
m=(p/'runtime_0_0_9_0G/source/main.c').read_text()
h=(p/'runtime_0_0_9_0G/include/sonic_gpu.h').read_text()
assert s.count('C3D_SyncDisplayTransfer(') >= 1
# No explicit second wait in texture upload (citro3d already waits).
assert 'gspWaitForPPF();' not in s
assert 'C3D_SyncDisplayTransfer ENTER' in s
assert 'C3D_SyncDisplayTransfer OK' in s
assert 'texture DONE' in s and 'Sonic GPU ready' in s
assert 'sa3ds_sonic_gpu_init_traced' in s and 'sa3ds_sonic_gpu_init_traced' in h
assert 'sa3ds_sonic_gpu_init(' in s and 'sa3ds_sonic_gpu_init(' in h
assert 'sa3ds_sonic_gpu_init_traced(&g_sonic_gpu' in m
assert 'log_texture_stage' in m
assert 'boot_log("TEX",line);' in m
assert 'sa1_physics_step_steam_sonic_flat(&g_player' in m
assert 'SA3DS Pass10A Pose Lab' in m and 'upload_rgba8_cpu' in s
print('TextureSyncFix source integration: PASS')
