#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.build_pc2004_emerald_coast_collision import build_collision_chunk, parse_collision_chunk, HEADER, ACT, MODEL, INSTANCE

if len(sys.argv)!=2:
    raise SystemExit('usage: test_ec_collision_chunk.py EXACT_2004_SONIC_EXE')
dec=PC2004StageDecoder(Path(sys.argv[1]))
blob,meta=build_collision_chunk(dec)
info=parse_collision_chunk(blob)
assert HEADER.size==52 and ACT.size==32 and MODEL.size==32 and INSTANCE.size==80
assert info=={'version':[1,0],'acts':3,'models':426,'instances':848,'vertices':30910,'indices':140709,'triangles':46903}
assert meta['source_addresses_in_runtime_payload'] is False
# Determinism: same exact input -> byte-identical neutral collision chunk.
blob2,meta2=build_collision_chunk(dec)
assert blob2==blob and meta2==meta
print('Emerald Coast pointer-free collision chunk tests: PASS')
