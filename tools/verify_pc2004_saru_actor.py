#!/usr/bin/env python3
"""Check actual-owner first orbit ticks and original E SARU handler identities."""
from pathlib import Path
import struct,subprocess,sys,tempfile,math
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.build_pc2004_saru_definitions import build
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def verify(exe,system,package):
    read=verified_reader(exe);assert Path(package).read_bytes()==build(exe,system)[0]
    entries=struct.unpack('<13I',read(0x4ad10c,52))
    assert entries==(0x4acfc6,0x4ad062,0x4ad070,0x4ad07c,0x4ad088,0x4ad096,0x4ad0a2,0x4ad0b0,0x4acfcf,0x4ad0c8,0x4ad0bc,0x4acfcf,0x4ad0da)
    for entry,target in zip(entries[:8],(0x4abcb0,0x4abef0,0x4abfd0,0x4ac090,0x4ac470,0x4ac170,0x4ac620,0x4ac250)):
        raw=read(entry,20);calls=[entry+i+5+struct.unpack_from('<i',raw,i+1)[0] for i in range(len(raw)-4) if raw[i]==0xe8];assert target in calls
    # Read actual constants instead of assuming the orbit coefficients.
    unit=struct.unpack('<d',read(0x7dccd0,8))[0];inverse=struct.unpack('<d',read(0x7dccc8,8))[0]
    add=struct.unpack('<f',read(0x7dccd8,4))[0];rates={4:struct.unpack('<f',read(0x7dce5c,4))[0],6:struct.unpack('<f',read(0x7dcb30,4))[0]}
    lut=read(0x88af78,4100);values=struct.unpack('<1025f',lut)
    def sample(i):
        i%=4096
        return values[i] if i<1024 else values[2048-i] if i<2048 else -values[i-2048] if i<3072 else -values[4096-i]
    def sine(a):
        a&=65535;i=a>>4;t=(a&15)/16.;return f32(sample(i)+(sample(i+1)-sample(i))*t)
    with tempfile.TemporaryDirectory(prefix='sa3ds-saru-actor-owner-') as temp:
        path=Path(temp)/'lut';path.write_bytes(lut)
        lines=subprocess.check_output(['bash',str(ROOT/'tests/run_saru_actor.sh'),str(package),str(path)],text=True).splitlines()
    rows=[(act,i,p,state) for act in (0,1) for i,p in enumerate(read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN')).records) if p.type_word&4095==29 for state in (4,6)]
    assert len(rows)==len(lines)==28;maximum=0.
    for (act,row,p,state),line in zip(rows,lines):
        v=line.split();assert list(map(int,v[:3]))==[act,row,state]
        radius=max(1.,p.parameters[2]);increment=math.trunc(1./(radius+add)*unit*(inverse if state==4 else -inverse));phase=p.angle_words[0]+increment
        assert int(v[3])==phase
        expected=(f32(p.position[0]+f32(sine(phase+16384)*radius)),p.position[1],f32(p.position[2]+f32(-sine(phase)*radius)))
        error=max(abs(f32(float(a))-b) for a,b in zip(v[4:7],expected));maximum=max(maximum,error);assert error<4e-5,(act,row,state,error)
        assert f32(float(v[7]))==rates[state] and int(v[8])==(0x670 if state==4 else 0x9a8)
    print(f'Original E SARU actor: PASS13-entry dispatch/source constants and28 owner first-orbit ticks, max_error={maximum}; floor query retained-Y fixture, no terrain/damage/projectile/native/retail proof')
if __name__=='__main__':
    if len(sys.argv)!=4:raise SystemExit('usage: verify_pc2004_saru_actor.py OWNER_SONIC_EXE OWNER_SYSTEM SARU_DEFINITIONS')
    verify(*map(Path,sys.argv[1:]))
