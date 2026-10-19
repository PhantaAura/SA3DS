#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder

if len(sys.argv)!=2:
    raise SystemExit('usage: test_pc2004_emerald_coast_decode.py EXACT_2004_SONIC_EXE')
dec=PC2004StageDecoder(Path(sys.argv[1]))
inv=dec.emerald_coast_inventory()
assert inv['emerald_coast_level_id']==1
assert inv['landtable_pointer_table_va']=='0x0097da08'
acts=inv['acts']
assert [a['landtable_va'] for a in acts]==['0x00e99cb8','0x01039e9c','0x010386b4']
assert [a['collision_count'] for a in acts]==[480,169,199]
assert [a['texture_archive'] for a in acts]==['BEACH01','BEACH02','BEACH03']
assert [a['aux_count'] for a in acts]==[0,0,0]
assert [a['load_flags'] for a in acts]==['0x000c']*3
assert [a['radius'] for a in acts]==[3000.0]*3
assert [a['unique_models'] for a in acts]==[183,142,101]
assert [a['source_points'] for a in acts]==[13323,10515,7072]
assert [a['meshsets'] for a in acts]==[578,557,314]
assert [a['triangles'] for a in acts]==[19598,16450,10855]
assert inv['totals']=={
    'collision_instances':848,
    'materials':1554,
    'meshsets':1449,
    'primitive_classes':{'0xc000':1449},
    'source_points':30910,
    'triangles':46903,
    'unique_models':426,
    'unique_objects':848,
}
# Every current EC collision entry is one flat object node with a valid Basic model.
for table in dec.emerald_coast_landtables():
    assert table.collision_list_va + table.collision_count*0x24 == table.va
    for row in dec.collisions(table):
        obj=dec.object(row.object_va)
        assert obj.model_va and not obj.child_va and not obj.sibling_va
        model=dec.model(obj.model_va)
        assert model.point_count>0 and model.meshset_count>0
print('PC2004 Emerald Coast exact-source decode tests: PASS')
