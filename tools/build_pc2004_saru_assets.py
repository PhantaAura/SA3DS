#!/usr/bin/env python3
"""Convert original E SARU rig, six referenced ACTIONs and textures, owner-side."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_chrmodels import SonicDecoder,ActionSummary
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.build_pc2004_sonic_sa3c import build_decoded_package
from tools.build_pc2004_saru_definitions import build as definitions
from tools import sa3t
# References in original4AB240..4AD140 plus constructor4AD140.
# Stable local slots; behavior chooses them at runtime, not by guessed names.
ACTIONS=(0x96a89c,0x9669ec,0x967964,0x968adc,0x969924,0x96b444)
class SaruDecoder(SonicDecoder):
    def actions(self):
        result=[]
        for i,address in enumerate(ACTIONS):
            root,motion=struct.unpack('<II',self.pe.read_bytes(address,8))
            nodes=self.hierarchy(root);m=self.motion(motion,len(nodes))
            if root!=0x965510 or len(nodes)!=38 or m.type_bits!=3 or m.interpolation!=2:
                raise ValueError('unexpected Saru ACTION profile')
            result.append(ActionSummary(i,address,root,motion,len(nodes),m))
        if [a.motion.frame_count for a in result]!=[16,16,16,16,13,7]:raise ValueError('unexpected Saru frame counts')
        return result

def build(exe,system):
    definitions(exe,system) # Exact executable/registry/code identity gate.
    stage=PC2004StageDecoder(exe)
    if stage.resolve_texture_archive(0x962560)!='E_SARU':raise ValueError('Saru texture list mismatch')
    dec=SaruDecoder.__new__(SaruDecoder);dec.pe=stage.pe
    pvm=PVMArchive(read_source(system,'E_SARU.PVM'),'E_SARU.PVM')
    character,report=build_decoded_package(dec,pvm,allow_enemy_primitives=True)
    entries=[dict(width=e.width,height=e.height,source_pixel_format=e.pixel_format,
        source_data_format=e.data_format,global_index=e.global_index,rgba=pvm.decode_base_rgba(e.index)) for e in pvm.entries]
    textures=sa3t.build(entries,pvm.sha256())
    report={'schema':'sa3ds-pc2004-saru-assets-v1','source_exe_sha256':dec.pe.sha256(),
        'source_pvm_sha256':pvm.sha256(),'decoder_revision':'pc2004-pvm9-linear-v1',
        'action_slots':[hex(a) for a in ACTIONS],'frame_counts':[16,16,16,16,13,7],
        'counts':report['counts'],'sa3c_bytes':len(character),'sa3c_sha256':hashlib.sha256(character).hexdigest(),
        'sa3t_bytes':len(textures),'sa3t_sha256':hashlib.sha256(textures).hexdigest(),
        'scope':'original rig/clips/Basic geometry/textures; AI, blend timing and native enemy lifecycle pending',
        'polygon_reference':'https://github.com/X-Hax/sa_tools/blob/master/Libraries/SAModel/Poly.cs'}
    return character,textures,report
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sonic_exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output_dir',type=Path);a=ap.parse_args()
    if a.output_dir.resolve().is_relative_to(ROOT):raise SystemExit('owner output must remain outside source tree')
    character,textures,report=build(a.sonic_exe,a.system);a.output_dir.mkdir(parents=True,exist_ok=True)
    (a.output_dir/'saru.sa3c').write_bytes(character);(a.output_dir/'saru.sa3t').write_bytes(textures)
    (a.output_dir/'saru-assets.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
