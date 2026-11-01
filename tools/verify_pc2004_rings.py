#!/usr/bin/env python3
"""Exact-owner static witnesses for the bounded native direct-ring and ring-group implementation.

No retail trace or complete object/collision parity is established by this tool.
Only short instruction witnesses and hashes are retained; owner assets/decompiled
bodies are not emitted. Values are read from the privately supplied executable.
"""
from pathlib import Path
import argparse
import configparser
import hashlib
import json
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader

# LLVM disassembly corroborated against the exact supported owner executable.
SIGNATURES={
    'Sonic two collision shapes':(0x49ABC1,'6a0268f0cb9100'),
    'Sonic radius from parameter rad':(0x49ABDA,'8b950c010000895114'),
    'Sonic current center_height':(0x4915F6,'8b911c010000'),
    'Sonic center full frame conversion':(0x49161A,'e871d6faff'),
    'Sonic center task XYZ addition':(0x491638,'d94720d80083c408d918d94724d8460cd95e0cd94728d84610d95e10'),
    'Sonic per-tick center call':(0x49BEFA,'e8f156ffff'),
    'ring collision init':(0x4503AC,'6a076a0168106b910053e835c7fcff'),
    'ring initial state0 to4':(0x4503A0,'c60604'),
    'ring state4 to5':(0x4503A6,'c60605'),
    'ring state5 to1':(0x45041D,'c60601'),
    'prior collision flag gate':(0x450466,'8b5638f64204010f84f3000000'),
    'collection consumes ring before tally':(0x450473,'6a00c60602e88316ffff'),
    'ring eligibility powerup flag':(0x45047D,'0fbf40062500400000'),
    'primary Sonic tally and sound7':(0x4504CB,'6a01e80e57fdff6a006a006a006a07e89138fdff'),
    'ring collision enqueue':(0x45057A,'56e800bdfcff'),
    'ring yaw increment364':(0x450586,'056c010000894618'),
    'ring pickup removal':(0x4506B9,'e892ba0100'),
    'SET consumed eligibility and active clear':(0x46C157,'8061037f8b481c806102fe'),
    'sphere dispatch call':(0x41FB03,'50565357e8f4aeffff'),
    'strict sphere overlap branch':(0x41AA7F,'ded9dfe0ddd8f6c4050f8a3a010000'),
    'signed16 ring addition':(0x425BE0,'668b0de4f0b003668bc166034c2404'),
    'ring counter write and sign extension':(0x425C03,'66890de4f0b0030fbfc9'),
    'hundreds increment lives only upwards':(0x425C21,'3bf27d0b2bd652e833ffffff'),
    'lives signed byte overflow saturates127':(0x425B7E,'001d34efb003790c6685db7e07c60534efb0037f'),
    'normal SET mode':(0x422961,'a0b48db10384c07407a144f49000eb0c'),
    'normal SET suffix by character':(0x422971,'e86a1bffff8b048524f49000'),
    'current character getter':(0x4144E0,'0fb705c02db203c3'),
    'SET class low12':(0x46BB0A,'81e1ff0f0000'),
    'SET clip selector high3':(0x46BB18,'c1e80c83e007'),
    'SET clip selector rules':(0x46BB21,'741b48a1f0dcab03740a83f8027c0ee98801000083f8010f8d7f010000'),
    'clip zero register':(0x4025D1,'33ed'),
    'clip getter default1 register':(0x4025FC,'bb01000000'),
    'clip key and section with default':(0x402790,'6890ca7d00536834ca7d00687cca7d00ffd6'),
    'clip clamp0..2':(0x4027A2,'3bc5a3f0dcab037d0433c0eb0a83f8027e0ab802000000a3f0dcab03'),
}
# Group/particle witnesses are separate from the portable runtime's scheduling.
SIGNATURES.update({
    'group count truncation plus1':(0x79B33B,'d9432c33ede8fb8feaff40'),
    'circle skips index0':(0x79B35E,'48752d3bee0f84bf000000'),
    'line odd index ceil and even negative half':(0x79B16B,'db4424107419d80d98cc7d0083ec08dd1c24e89e99eaffd84e3083c408eb09d84e30d80de4ff7d00'),
    'circle float degree scale':(0x79B228,'d80dd8007e00'),
    'circle float intermediate and original denominator':(0x79B236,'d9542414d8762c'),
    'circle BAMS double scale then truncation':(0x79B23D,'dc0dd0cc7d00dc0da0f17d00e8f290eaff'),
    'group rotateZ thenX thenY':(0x79B1B5,'0fb7d033c9e8f19dfeff8b461485c0740a0fb7d033c9e8109dfeff8b461885c0740a0fb7d033c9e83f9cfeff'),
    'group child Ring_Main allocation':(0x79B3D2,'5768700345006a02e86105c7ff'),
    'group child original index mask':(0x79B3EA,'b2018bcdd2e28b482088510c'),
    'group zero child rotation':(0x79B417,'33c9894a148b5020894a188b402089481c'),
    'group init enters action1':(0x79B447,'c60301'),
    'group updates children then journals':(0x79B4A0,'57e87affc6ff83c404e852fcffff'),
    'group journal OR child mask':(0x79B110,'57e87a10cdff8b4e2033d28a510c0ad05257e84910cdff'),
    'child prepend order':(0x40B95C,'8b510c89108b510c85d2740389420489410c'),
    'group child-distance unload handling':(0x79B4C0,'39481075098b5020807a0d0174088b003bc675eceb03c60303'),
    'group action3 becomes4':(0x79B56A,'c60304'),
    'group action4 deletes parent':(0x79B572,'57e84801c7ff'),
    'group destructor deletes children':(0x4FB290,'e94b05f1ff'),
    'pickup initial three ages':(0x450420,'c7460800000000c7462c000000c0c74634000080c0'),
    'pickup age update pause gate':(0x44F7A3,'e8c855fcff85c07524'),
    'pickup three float32 age additions':(0x44F7AC,'d94608d8055cce7d00d95e08d9462cd8055cce7d00d95e2cd94634d8055cce7d00d95e34'),
    'pickup completion behind render guard':(0x44F7D0,'a17c11b10385c00f8543010000'),
    'pickup last age integer conversion':(0x44F81E,'e81d4b1f00'),
    'pickup last integer age greater7 becomes3':(0x44F90A,'db44241c5f5bd81dfcf17d00dfe0f6c4417503c60603'),
    'pickup Main action2 helper call':(0x4506A9,'e8f2f0ffff'),
    'pickup display callback helper call':(0x44FC88,'e813fbffff'),
    'source pause states10 and19':(0x414D70,'66a1e42db203663d10007409663d1900740333c0c3b801000000c3'),
    'ordinary update then collision pass':(0x413D58,'e81374ffffe89ec90000'),
    'alternate display-only pass':(0x413F25,'c7057c11b10300000000e80c76ffff'),
    'sine index and low4 fraction':(0x788293,'c1f90481e1ff0f00008bc183e20f'),
    'sine interpolation subtraction':(0x788397,'85d2d8e189542404db442404'),
    'sine interpolation factor and result':(0x7883AB,'d80d20017e005edec9dec1'),
})
# All ends are EXCLUSIVE; hashes include each bounded function's final RET.
FUNCTION_DIGESTS={
    'group task':(0x79B2F0,0x79B583,'99ec4630cc1a2baa2e38dd489e200928d4680773af90dd6631c83f07689fd352'),
    'group line helper':(0x79B140,0x79B205,'29f6a77173c1b2d9c3c552497e3e86d57f4b4b779256a9278fb7f3b4158c023d'),
    'group circle helper':(0x79B210,0x79B2EF,'84bd6f08408bf8aeb2ea14295ccb068aac11d0dcdae8224872f271d24fd53d76'),
    'group journal':(0x79B100,0x79B134,'067245896f4d310a9eeeb2de96021fa26020a74735fc5efa63a583487bdffe17'),
    'Ninja sine':(0x788290,0x7883BD,'8f1b0027572069212bd02ff6022eb649cbef80939000c432de05f82426e9107f'),
    'Ninja cosine':(0x7883C0,0x7884ED,'67e1d46613a225b2681c018b61a429336b45c8d8bb78b90d21c6af624df76569'),
}
QUARTER_LUT_SHA256='3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed'

ADD_RINGS_SHA256='e264dcbdc4316e998eb21756f458840cddb72122e9b74c461e496c86daf2e809'

def verify(path):
    read=verified_reader(path)
    for label,(address,hexbytes) in SIGNATURES.items():
        expected=bytes.fromhex(hexbytes)
        if read(address,len(expected))!=expected:
            raise ValueError('ring instruction witness mismatch: '+label)
    for label,(start,end,digest) in FUNCTION_DIGESTS.items():
        if hashlib.sha256(read(start,end-start)).hexdigest()!=digest:
            raise ValueError('ring bounded function identity mismatch: '+label)
    # Covers the complete bounded function, including absence of a999 clamp.
    if hashlib.sha256(read(0x425BE0,0x425C4D-0x425BE0)).hexdigest()!=ADD_RINGS_SHA256:
        raise ValueError('AddRings bounded function identity mismatch')
    def u32(a):return struct.unpack('<I',read(a,4))[0]
    def f32(a):return struct.unpack('<f',read(a,4))[0]
    if (u32(0x41FBC8+7*4),u32(0x41FBEC))!=(0x41FA57,0x41FB03):
        raise ValueError('form7 versus form0 dispatch mismatch')
    if (f32(0x9154E8+28*4),f32(0x9154E8+32*4))!=(4.0,struct.unpack('<f',struct.pack('<f',5.4))[0]):
        raise ValueError('Sonic collision radius/center parameters mismatch')
    if read(0x91CBF0+4,4)!=struct.pack('<I',0x420):
        raise ValueError('Sonic primary collision world-center attributes mismatch')
    if read(0x916B10+8,12)!=bytes(12) or f32(0x916B10+20)!=5.0:
        raise ValueError('ring collision sphere mismatch')
    if u32(0x90F424)!=0x7DDEBC or read(0x7DDEBC,2)!=b'S\0':
        raise ValueError('normal Sonic SET suffix mismatch')
    if read(0x7DCA34,10)!=b'cliplevel\0' or read(0x7DCA7C,8)!=b'sonicDX\0':
        raise ValueError('source clipping setting name mismatch')
    registry=struct.unpack('<BBHfIII',read(0x1036420+21*20,20))
    if registry[:5]!=(2,3,1,360000.0,0) or registry[5]!=0x79B2F0:
        raise ValueError('Emerald Coast ring-group registry mismatch')
    quarter=read(0x88AF78,1025*4)
    if hashlib.sha256(quarter).hexdigest()!=QUARTER_LUT_SHA256:
        raise ValueError('Ninja quarter-wave table identity mismatch')
    if struct.unpack('<f',quarter[:4])[0]!=0 or struct.unpack('<f',quarter[-4:])[0]!=1:
        raise ValueError('Ninja quarter-wave endpoints mismatch')
    if (f32(0x7DCC98),f32(0x7DFFE4),f32(0x7E00D8),f32(0x7E0120))!=(.5,-.5,360.0,1/16):
        raise ValueError('group formula/angle interpolation constants mismatch')
    if (struct.unpack('<d',read(0x7DCCD0,8))[0],struct.unpack('<d',read(0x7DF1A0,8))[0])!=(65536.0,1/360):
        raise ValueError('circle BAMS conversion constants mismatch')
    step=f32(0x7DCE5C);limit=f32(0x7DF1FC)
    if step!=struct.unpack('<f',struct.pack('<f',.8))[0] or limit!=7:
        raise ValueError('pickup age constants mismatch')
    # Derived ordinary-rendering lifecycle, not a universal wall-clock timer.
    age=-4.0;updates=0
    while int(age)<=limit and updates<100:
        age=struct.unpack('<f',struct.pack('<f',age+step))[0];updates+=1
    if updates!=15:raise ValueError('derived pickup lifecycle mismatch')
    return {'instruction_witnesses':len(SIGNATURES),'sonic_radius':4.0,
            'sonic_center_height':f32(0x9154E8+32*4),'ring_radius':5.0,
            'normal_sonic_set_suffix':'S','cliplevel_fallback':1,
            'ring_group_class':21,'group_function_digests':len(FUNCTION_DIGESTS),
            'owner_quarter_lut_bytes':len(quarter),'pickup_age_step':step,
            'pickup_render_enabled_updates_to_action3':updates,
            'limitations':'Static witnesses only; mixed rendering/simulation phase, particles, complete checkpoint transitions and independent retail-frame traces remain unverified',
            'authority':'EXACT-OWNER STATIC INSTRUCTIONS/DATA; NOT RETAIL TRACE'}

def ini_clip(path):
    """Report this owner's readable integer setting; do not alter their INI."""
    parser=configparser.ConfigParser()
    if not path.is_file():return {'path':str(path),'present':False,'cliplevel':1}
    parser.read_string(path.read_text(encoding='utf-8-sig'))
    section=next((name for name in parser.sections() if name.lower()=='sonicdx'),None)
    value=parser.get(section,'cliplevel',fallback='1') if section else '1'
    try:value=int(value,10)
    except ValueError:raise ValueError('owner cliplevel is not a supported integer')
    return {'path':str(path),'present':True,'cliplevel':min(2,max(0,value))}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe',type=Path)
    parser.add_argument('--ini',type=Path)
    args=parser.parse_args()
    report=verify(args.sonic_exe)
    report['owner_ini']=ini_clip(args.ini or args.sonic_exe.parent/'sonicDX.ini')
    print('PC2004 direct rings/groups: PASS '+json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
