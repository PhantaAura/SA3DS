#!/usr/bin/env python3
"""Check collision contact rules with the owner's actual Sonic/Saru records."""
from pathlib import Path
import hashlib,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={(4827720, 168): '6cda48624ba1d8ac35166f7142bbda3c06494277a13ae56d6b43e4d9960486c2', (4295520, 128): '5cffe913669538172f022d1f5e775ec474fd7fff907663d906f6fd5e83a44093', (4295936, 128): 'ed332dbe1220f5b43ac7d2297715bb3e8e1f3ab10ef985eaa3e0ab9f9957f8fa', (4296160, 135): '2550e20e4bf99303db9b7df73cee45c4ee064200d7a88b2f84316d15662d4430', (4296304, 272): 'f0f129670788b49971c62f28bdc4a026af2ef20653be56d88790ff2988f96c73', (4303360, 480): '90e4a2219a220adea040ad0b52cfaa18537774e19474286f332acea5d6af4cdb', (4325056, 336): '8aa472970d8d22db778ff5cf96fd70465e88b8d1b90cdba84a68955978e323ef', (4296064, 83): '8dac540e46d5d7c76e411b75a92299a0a55834fb9b2f450924974bacefa8134d', (4295360, 89): '9a213f76f24e32084e219b0b1e895596a21627b79b0589fe76b02f9405a5b205'}
def verify(exe):
    read=verified_reader(exe)
    for (address,size),digest in RANGES.items():assert hashlib.sha256(read(address,size)).hexdigest()==digest
    primary=read(0x91cbf0,48);enemy=read(0x96c9e8,48);secondary=read(0x91cc20,48)
    masks=read(0x90e3c8,10);excluded=struct.unpack('<10I',read(0x90e3d4,40))
    profile=primary+enemy+struct.pack('<BB2xIII',masks[0],masks[3],excluded[0],excluded[3],0)+secondary
    assert len(profile)==160
    with tempfile.TemporaryDirectory(prefix='sa3ds-ccl-owner-') as directory:
        path=Path(directory)/'owner-records.bin';path.write_bytes(profile)
        lines=subprocess.check_output(['bash',str(ROOT/'tests/run_ccl_contact.sh'),str(path)],text=True).splitlines()
    assert len(lines)==4
    for mode,line in enumerate(lines):
        values=list(map(int,line.split()));assert values[0]==mode
        attack,defense=((0,0),(1,1),(1,3),(3,3))[mode]
        pd=(primary[3]&240)|attack|(defense<<2);ed=enemy[3]
        player_hurt=(ed&3)>defense and bool(masks[0]&ed)
        enemy_hurt=attack>((ed>>2)&3) and bool(masks[3]&pd)
        pstatus=(0x100 if mode==1 else 0)|8|(4 if player_hurt else 0);estatus=0x100|8|(4 if enemy_hurt else 0)
        if player_hurt:pd|=16
        if enemy_hurt:ed|=16
        primary_attributes=struct.unpack_from('<I',primary,4)[0];enemy_attributes=struct.unpack_from('<I',enemy,4)[0]
        assert not excluded[0]&enemy_attributes and not excluded[3]&primary_attributes
        assert values[1:]==[1,1,1,pstatus,estatus,pd,ed,50,1,struct.unpack_from('<I',secondary,4)[0]|16],(mode,values)
    # Actual pair cannot push either task under418D80's category/push masks.
    receive=read(0x90e3bc,10);inhibit=read(0x90e3b0,10)
    def push(own,other,category,other_category):
        return bool(receive[category]&other[2]) and not bool(inhibit[other_category]&own[2])
    assert not push(primary,enemy,0,3) and not push(enemy,primary,3,0)
    print(f'Original CCL contact: PASS{len(RANGES)} helper identities +4 actual Sonic/Saru strength/contact cases, secondary mask and no-push profile; static/host only, not native scheduling or retail execution')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_ccl_contact.py OWNER_SONIC_EXE')
    verify(Path(sys.argv[1]))
