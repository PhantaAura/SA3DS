"""Host source contract only: this is not a native devkitARM compile."""
from pathlib import Path
p = Path('runtime_0_0_9_0G/source/main.c').read_text()
mk = Path('runtime_0_0_9_0G/Makefile').read_text()
assert 'SOURCES     := source ../src/sa3c ../src/sa1/physics ../src/game' in mk
assert 'sa3ds_wz_tick_begin(&g_tick_clock,elapsed_ms,&in)' in p
assert 'sa3ds_wz_tick_input(&g_tick_clock,step)' in p
assert 'for (unsigned step=0;step<steps;step++)' in p
assert 'sa1_physics_step_steam_sonic_flat(&g_player,&tick_input' in p
assert 'sa3ds_parity_trace_write_frame(g_trace_file,g_selected_zone' in p
assert 'sa3ds_wz_tick_reset(&g_tick_clock)' in p
assert 'g_paused=!g_paused' in p
assert 'if (memcmp(&physics_done,&g_player,sizeof(g_player))!=0)' in p
print('0G fixed-step runtime source integration: PASS (not native compiled)')
