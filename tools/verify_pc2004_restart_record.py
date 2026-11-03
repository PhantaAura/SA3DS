#!/usr/bin/env python3
"""Exact-owner restart/checkpoint contracts; hashes and addresses, no game code."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x44ed50,17):'f79427952e6412add15411344c1b741d6212a10e112e82c877b7866e3b4d0b66',
 (0x44ed70,52):'ca427f96c6246a64ab02f96a798a897f05b6a74c25326422ffb7f97f2fa003d5',
 (0x44edb0,184):'3ed3192ef73ffb7b7b8cdc9cf57060e4a2b05f5fb11aa4b17ad9db3fdbe1ea2a',
 (0x44ee70,114):'28e3936376a3fc138dc15d652713220a7b97b9100caed2ba875521f046a553be',
 (0x4265f0,30):'5091d06b6e6b8e11cba27ccb8da22ee6c3645698a62799c431c6b2cca52a7d5d',
 (0x426610,36):'878f76501d2753048ee92a30b06d11484dd1f32785292850025b795873d2d26b',
 (0x414810,745):'39202b6cad12269418033af3ef9fe81b6880da134a2f783d459f919c58cceed0',
 (0x44f430,263):'5c30102d421f0b32497ea4bf85f9b75bf6e1ee6f1f7f1dc499fdc72165c52e4e',
 (0x44f540,344):'faaf13a2c109a3221f37498c7787d7a7a30f6d597c7e9f0ab6640dc979f3e3b3',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 registry=struct.unpack('<BBHfIII',r(0x1036420+19*20,20))
 assert registry[:6]==(15,6,1,360000.,0,0x44f540)
 for a,target in [(0x44ee0a,0x4265f0),(0x44ee0f,0x425cc0),(0x44ee17,0x425be0),
                  (0x44ee1e,0x441fd0),(0x44ee7f,0x426610),(0x44f511,0x44ee70)]:
  assert r(a,1)==b'\xe8' and a+5+struct.unpack('<i',r(a+1,4))[0]==target,hex(a)
 assert struct.unpack('<5I',r(0x44f684,20))==(0x44f578,0x44f624,0x44f635,0x44f654,0x44f66c)
 print('Restart record owner witnesses: PASS exact SHA,9 code spans,clock/ring reset calls,checkpoint registry19 and task dispatcher; native activation/visuals remain separate')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_restart_record.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
