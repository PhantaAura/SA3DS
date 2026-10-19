import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import recon_pc2004_character_loader as r
exe=Path('/mnt/data/sonic.exe')
proxy=Path('/mnt/data/SADXModLoader.dll')
if not exe.exists(): raise SystemExit('SKIP: /mnt/data/sonic.exe unavailable')
a=r.analyze_exe(exe)
m={x['name']:x for x in a['sonic_exports']}
assert a['imports']['LoadLibraryA_iat_va']==0x7DB040
assert a['imports']['GetProcAddress_iat_va']==0x7DB044
assert a['chrmodels_load']['loader_function_va']==0x401AC0
assert m['___SONIC_MODELS']['receiving_global_va']==0x038F6EE8
assert m['___SONIC_OBJECTS']['receiving_global_va']==0x038F6E80
assert m['___SONIC_ACTIONS']['receiving_global_va']==0x038F6E7C
assert m['___SONIC_MOTIONS']['receiving_global_va']==0x03A8AF70
if proxy.exists():
    p=r.analyze_modloader(proxy)
    assert p['normal_pe_export_directory']['is_empty']
    assert p['proven_sonic_table_counts']=={'models':11,'objects':79,'actions':149}
    assert p['proven_object_layout']['model_ptr_offset']==4
    assert p['proven_object_layout']['child_ptr_offset']==0x2c
    assert p['proven_object_layout']['sibling_ptr_offset']==0x30
    assert p['proven_action_layout']['object_ptr_offset']==0
    assert p['proven_action_layout']['motion_ptr_offset']==4
print('PC2004 character-loader recon tests: PASS')
