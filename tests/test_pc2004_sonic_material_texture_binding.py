#!/usr/bin/env python3
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_chrmodels import SonicDecoder
from tools.importers.pc2004_pvm import SonicPVM


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit('usage: test_pc2004_sonic_material_texture_binding.py CHRMODELS.DLL SONIC.PVM')
    dec=SonicDecoder(Path(sys.argv[1]))
    pvm=SonicPVM(Path(sys.argv[2]))
    actions=dec.actions()
    roots=sorted({a.root_object_va for a in actions if a})
    nodes={}
    for root in roots:
        for obj in dec.hierarchy(root):
            nodes[obj.va]=obj
    model_vas=sorted({o.model_va for o in nodes.values() if o.model_va})
    used=set()
    for model_va in model_vas:
        raw=dec.pe.read_bytes(model_va,44)
        _points,_normals,_npts,_meshes,materials_va,_nmesh,nmat=struct.unpack_from('<IIIIIHH',raw,0)
        for i in range(nmat):
            mat=dec.material(materials_va+i*20)
            assert 0 <= mat.texture_id < pvm.texture_count
            used.add(mat.texture_id)
    assert used == {0,1,2,3,4,5,6,7,9,10,13,14,15}
    print('PC2004 Sonic material -> SONIC.PVM binding: PASS (13/16 slots referenced)')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
