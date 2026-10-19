#!/usr/bin/env python3
"""Source-only BootTrace1 guard (NOT real 3DS compile or hardware test)."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
make=(root/'runtime_0_0_9_0G/Makefile').read_text()
checks={
 '3-mode startup': ('boot_menu()' in main and 'if (mode==1)' in main and 'if (mode==2)' in main),
 'A full WIP9 retained': ('if (!init_scene())' in main and 'sa1_physics_step_steam_sonic_flat(&g_player' in main and 'sa3ds_wz_tick_begin(&g_tick_clock' in main),
 'GPU-only independent assets': ('if (mode==1)' in main and main.index('if (mode==1)')<main.index('if (mode==2)') and '0x3060B0FFu' in main),
 'asset-only preflight': ('if (mode==2)' in main and 'if (!load_owner_packages())' in main),
 'log checkpoint': ('pass10_boot.log' in main and 'boot_checkpoint("FULL: C3D_Init")' in main and 'boot_checkpoint("CPU swizzle Sonic textures")' in main),
 'visible bottom console': ('gfxSetDoubleBuffering(GFX_BOTTOM,false)' in main and 'gfxScreenSwapBuffers(GFX_BOTTOM,false)' in main),
 'no whole-screen swaps': ('gfxSwapBuffers();' not in main),
 'original 60Hz/asset telemetry': ('60Hz:%llu Drop:%llu' in main and 'Assets:%s C:%luK T:%luK' in main),
 'source GPU linked': ('-lcitro3d' in make and 'ROMFS       := romfs_sd' in make),
}
for label,ok in checks.items():
 assert ok,label
print('WIP9 BootTrace1 source integration: PASS (all {} checks)'.format(len(checks)))
