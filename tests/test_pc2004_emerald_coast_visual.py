#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder,EMERALD_COAST_LEVEL_ID
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.pc2004_pvm import SonicPVM
from tools.build_pc2004_emerald_coast_visual import read_source,build_visual_chunk,parse_visual_chunk,build_material_chunk,parse_material_chunk,build_act_package
from tools import sa3dspak,sa3t
if len(sys.argv)!=3:raise SystemExit('usage: test_pc2004_emerald_coast_visual.py EXACT_2004_SONIC_EXE SYSTEM_DIR_OR_ZIP')
exe=Path(sys.argv[1]);src=Path(sys.argv[2]);dec=PC2004StageDecoder(exe)
expected={
 'BEACH01.PVM':(103,'fb6fa479787fb29e57688ea56cf21da6952142d9ebb98665895d9c1372105a07'),
 'BEACH02.PVM':(91,'d871b7a7e78d8ea96c93a77e77290a7a2da3630942e40fcd7c744875d8a8e866'),
 'BEACH03.PVM':(89,'01fc98834be904a173a7aba7a3f18ef88418f9460bb78c4cfd9aba99864e6fb3'),
 'OBJ_BEACH.PVM':(90,'8b64b5b454dc7fac3474932dde42d749a46e406c4e756865b21e7d134227d50c'),
 'BEACH_SEA.PVM':(17,'0b9ee8d893cdb5b3d3f8a8e9d035292559159295d2b0bdd8683e7a190b67668d'),
 'BG_BEACH.PVM':(3,'6ee04c83f990066cec8cad4763e3ee8176bf05ebb277ad9d337e996488d718a4'),
}
pvms={}
for name,(count,sha) in expected.items():
 p=PVMArchive(read_source(src,name),name);pvms[name]=p
 assert p.texture_count==count and p.sha256()==sha
 assert {e.data_format for e in p.entries}=={9}
 assert {e.pixel_format for e in p.entries}<={0,1,2}
 # Decode first/last base mip to ensure twiddle/pixel path covers archive.
 for i in {0,p.texture_count-1}:
  rgba=p.decode_base_rgba(i);e=p.entries[i];assert len(rgba)==e.width*e.height*4
# Strong base-first witness. BEACH01 is base-only for this texture while
# BEACH03 stores a mip chain; their largest image is byte-identical.
a=pvms['BEACH01.PVM'];b=pvms['BEACH03.PVM']
ma={e.name:e for e in a.entries};mb={e.name:e for e in b.entries}
assert a.decode_base_rgba(ma['m128_cos002'].index)==b.decode_base_rgba(mb['m128_cos002'].index)
expected_visual={
 0:(183,578,480,22237,58794,19598,237,61,103),
 1:(142,557,169,16460,49350,16450,120,25,91),
 2:(101,314,199,10418,32565,10855,122,14,89),
}
for act,want in expected_visual.items():
 table=dec.landtable(EMERALD_COAST_LEVEL_ID,act);p=pvms[table.texture_archive+'.PVM']
 vis,mats,meta=build_visual_chunk(dec,act,p.texture_count);v=parse_visual_chunk(vis)
 got=(v['models'],v['meshes'],v['instances'],v['vertices'],v['indices'],v['triangles'],len(mats),meta['counts']['meshes_without_uv'],p.texture_count)
 assert got==want,(act,got,want)
 assert meta['uv']['divisor']==255.0
 # All current EC textured material IDs are valid local PVM indices.
 for key in mats:assert 0<=key[3]<p.texture_count
 mat_blob=build_material_chunk(mats);assert parse_material_chunk(mat_blob)['count']==len(mats)
 pak,report=build_act_package(dec,act,p.data);info=sa3dspak.parse(pak)
 assert [c['type'] for c in info['chunks']]==['METADATA','GEOMETRY','MATERIALS','TEXTURES']
 tex=[c for c in info['chunks'] if c['type']=='TEXTURES'][0]['data'];assert len(sa3t.parse(tex)['textures'])==p.texture_count
print('PC2004 Emerald Coast visual/material/PVM decode tests: PASS')
