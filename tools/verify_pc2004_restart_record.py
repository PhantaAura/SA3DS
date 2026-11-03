#!/usr/bin/env python3
"""Exact-owner restart/checkpoint contracts; hashes and addresses, no game code."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x46c360,36):'116bdfe9a2f5a1158436beff13d09f4f4e40a9d80b1f42836f159de85b8fb775',
 (0x46c010,224):'3f4159bac0b596e36e24c0b53cbf638bade4ec0cc8dea624f489c1258628271a',
 (0x46ba40,670):'ee5c00de3f27fbaae136a27860e1913a01da0552c6c1eda937d041af882fce8f',
 (0x46b750,88):'a33ac8b39b2e75c5425effe386ba0bfaf43bdc35db21c56aa6cf1e348072d08a',
 (0x46c0f0,43):'0d089041ed1b92f25b273f15d2d02a638d66358c4a063a82e7b2ca59d7682f01',
 (0x44f3b0,128):'62fa31f920a48489e15da99a7fbab3a53c10236fd947e3a893d89ab15e4ceb30',
 (0x44f2d0,197):'1cf21c35352cd85dbb1642bead3fb7f476bbba2626e98dd02c3095167ac7fd7b',
 (0x44f150,380):'026f1eb38ec8513e439cda7fccc4c0c432124710197f305495c4a7eb820f6a57',
 (0x44f070,212):'065fcfad965ac06ad6427e041c686e893ea7108739bafd536bfad55efd30c056',
 (0x44efe0,140):'f23a22c0dc57f7f4b484fd538940bd097ecd10143105afd11a550d8267403718',
 (0x44eef0,208):'85a6640f5596df6559726e1955268c758029b445f9d6606a0e2b606e33a46521',
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
                  (0x44ee1e,0x441fd0),(0x44ee7f,0x426610),(0x44f511,0x44ee70),(0x44f54a,0x46c360)]:
  assert r(a,1)==b'\xe8' and a+5+struct.unpack('<i',r(a+1,4))[0]==target,hex(a)
 assert struct.unpack('<5I',r(0x44f684,20))==(0x44f578,0x44f624,0x44f635,0x44f654,0x44f66c)
 assert struct.unpack('<f',r(0x7e05b4,4))[0]==0.9950000047683716
 assert struct.unpack('<f',r(0x7e05b0,4))[0]==-13421773/1099511627776
 assert struct.unpack('<f',r(0x7dcb34,4))[0]==10.
 assert struct.unpack('<d',r(0x7dccd0,8))[0]==65536.
 assert struct.unpack('<d',r(0x7df1a0,8))[0]==1./360.
 print('Restart record owner witnesses: PASS exact SHA,20 code spans,clock/ring reset calls,checkpoint registry19 and task dispatcher; native activation/visuals remain separate')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_restart_record.py OWNER_SONIC_EXE')
 verify(Path(sys.argv[1]))
