#!/usr/bin/env python3
"""Static native integration contract; does NOT claim a devkitARM build."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'runtime_0_0_9_0G/source/main.c').read_text()
mk=(root/'runtime_0_0_9_0G/Makefile').read_text()
loader=(root/'runtime_0_0_9_0G/source/owner_assets.c').read_text()
assert 'sa3ds_load_owner_pair("romfs:/sa3ds", "sdmc:/3ds/SA3DS/assets"' in main
assert 'sa3ds_owner_source_name(g_owner_source)' in main
assert 'sa3c_character_open(' in main and 'sa3ds_visual_preflight(' in main
assert 'ASSET_SOURCE),sd' in mk
assert 'check-assets:' in mk and 'sonic.sa3c' in mk and 'sonic.sa3t' in mk
assert 'SA3DS_OWNER_ASSET_MAX_BYTES' in loader
assert 'free(*c); *c = NULL; *cs = 0;' in loader
print('0G external SD pair integration: PASS (source contract, not native build)')
