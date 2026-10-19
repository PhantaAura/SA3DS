#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

PC2004_EXE_SIZE=55549952
PC2004_EXE_SHA256='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
MODLOADER_SIZE=1031680
MODLOADER_SHA256='3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
ORIG_CHRMODELS_SIZE=7647232
ORIG_CHRMODELS_SHA256='76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5'

class PE32:
    def __init__(self,path):
        self.path=Path(path); self.data=self.path.read_bytes()
        if self.data[:2]!=b'MZ': raise ValueError('not MZ')
        pe=struct.unpack_from('<I',self.data,0x3c)[0]
        if self.data[pe:pe+4]!=b'PE\0\0': raise ValueError('not PE')
        self.pe=pe; coff=pe+4
        self.machine,self.nsec=struct.unpack_from('<HH',self.data,coff)
        self.optsz=struct.unpack_from('<H',self.data,coff+16)[0]
        self.opt=coff+20
        if struct.unpack_from('<H',self.data,self.opt)[0]!=0x10b: raise ValueError('not PE32')
        self.image_base=struct.unpack_from('<I',self.data,self.opt+28)[0]
        self.entry_rva=struct.unpack_from('<I',self.data,self.opt+16)[0]
        self.sections=[]
        so=self.opt+self.optsz
        for i in range(self.nsec):
            o=so+i*40
            name=self.data[o:o+8].rstrip(b'\0').decode('ascii','replace')
            vsize,rva,rawsize,raw=struct.unpack_from('<IIII',self.data,o+8)
            self.sections.append(dict(name=name,rva=rva,vsize=vsize,raw=raw,rawsize=rawsize))
        self.data_dirs=[]
        dd=self.opt+96
        for i in range(16): self.data_dirs.append(struct.unpack_from('<II',self.data,dd+i*8))

    def rva_to_off(self,rva):
        for s in self.sections:
            if s['rva'] <= rva < s['rva']+max(s['vsize'],s['rawsize']):
                x=rva-s['rva']
                if x>=s['rawsize']: raise ValueError('RVA in zero-fill')
                return s['raw']+x
        if rva < min((s['raw'] for s in self.sections), default=0x1000): return rva
        raise ValueError(f'unmapped RVA {rva:#x}')
    def va_to_off(self,va): return self.rva_to_off(va-self.image_base)
    def off_to_va(self,off):
        for s in self.sections:
            if s['raw']<=off<s['raw']+s['rawsize']:
                return self.image_base+s['rva']+(off-s['raw'])
        raise ValueError('unmapped file offset')
    def cstr_va(self,va):
        o=self.va_to_off(va); e=self.data.find(b'\0',o)
        return self.data[o:e].decode('ascii','replace')
    def wstr_va(self,va,max_chars=256):
        o=self.va_to_off(va); b=self.data[o:o+max_chars*2]
        chars=[]
        for i in range(0,len(b)-1,2):
            c=struct.unpack_from('<H',b,i)[0]
            if c==0: break
            chars.append(chr(c))
        return ''.join(chars)
    def sha256(self): return hashlib.sha256(self.data).hexdigest()
    def export_dir(self): return self.data_dirs[0]
    def import_iat(self):
        rva,size=self.data_dirs[1]
        if not rva: return {}
        out={}; off=self.rva_to_off(rva)
        while True:
            oft,timestamp,fwd,name_rva,ft=struct.unpack_from('<IIIII',self.data,off)
            if not any((oft,timestamp,fwd,name_rva,ft)): break
            no=self.rva_to_off(name_rva); ne=self.data.find(b'\0',no)
            dll=self.data[no:ne].decode('ascii','replace')
            thunk_rva=oft or ft; to=self.rva_to_off(thunk_rva); idx=0
            while True:
                val=struct.unpack_from('<I',self.data,to+idx*4)[0]
                if val==0: break
                if not (val & 0x80000000):
                    io=self.rva_to_off(val); fn0=io+2; fn1=self.data.find(b'\0',fn0)
                    fn=self.data[fn0:fn1].decode('ascii','replace')
                    out[(dll.lower(),fn)]=self.image_base+ft+idx*4
                idx+=1
            off+=20
        return out

def expect_bytes(pe,va,expected,label):
    off=pe.va_to_off(va)
    got=pe.data[off:off+len(expected)]
    if got!=expected: raise ValueError(f'{label}: byte witness mismatch at {va:#x}: {got.hex()} != {expected.hex()}')
    return got.hex()

def find_ascii(pe,s):
    b=s.encode()+b'\0'; fo=pe.data.find(b)
    if fo<0: raise ValueError(f'missing string {s}')
    return pe.off_to_va(fo)

def all_abs_refs(pe,va):
    pat=struct.pack('<I',va); out=[]; pos=0
    while True:
        p=pe.data.find(pat,pos)
        if p<0: return out
        try: out.append(pe.off_to_va(p))
        except ValueError: pass
        pos=p+1

def map_loader_export(pe,name,search_start=0x401ae0,search_end=0x401c80):
    nameva=find_ascii(pe,name)
    start=pe.va_to_off(search_start); end=pe.va_to_off(search_end)
    push=b'\x68'+struct.pack('<I',nameva)
    p=pe.data.find(push,start,end)
    if p<0: raise ValueError(f'no loader xref for {name}')
    window=pe.data[p:p+40]
    call=window.find(b'\xff\xd7')
    if call<0: raise ValueError('no GetProcAddress call')
    store=window.find(b'\xa3',call+2)
    if store<0 or store+5>len(window): raise ValueError('no receiving-global store')
    glob=struct.unpack_from('<I',window,store+1)[0]
    return {'name':name,'name_va':nameva,'push_va':pe.off_to_va(p),'receiving_global_va':glob}

def analyze_exe(path):
    pe=PE32(path)
    if len(pe.data)!=PC2004_EXE_SIZE or pe.sha256()!=PC2004_EXE_SHA256: raise ValueError('2004 sonic.exe fingerprint mismatch')
    imports=pe.import_iat()
    ll=imports.get(('kernel32.dll','LoadLibraryA')); gp=imports.get(('kernel32.dll','GetProcAddress'))
    chrpath=find_ascii(pe,r'SYSTEM\chrmodels.dll')
    loader_xrefs=all_abs_refs(pe,chrpath)
    sonic=[map_loader_export(pe,n) for n in ('___SONIC_MODELS','___SONIC_OBJECTS','___SONIC_ACTIONS','___SONIC_MOTIONS')]
    # Validate runtime byte witnesses rather than merely recording hard-coded addresses.
    runtime_bytes={
      'action_object_field_read': expect_bytes(pe,0x494426,b'\x8b\x01','ACTION +0 object read'),
      'action_table_pointer_compare': expect_bytes(pe,0x49442c,b'\x8b\x15\x7c\x6e\x8f\x03\x3b\x0a','ACTION table compare'),
      'action_motion_field_read': expect_bytes(pe,0x494464,b'\x8b\x51\x04','ACTION +4 motion read'),
      'sonic_motion_table_special_case': expect_bytes(pe,0x4945a8,b'\x8b\x15\x70\xaf\xa8\x03\x8b\x42\x04','MOTION table index'),
    }
    return {
      'file':{'size':len(pe.data),'sha256':pe.sha256(),'image_base':pe.image_base,'entry_va':pe.image_base+pe.entry_rva},
      'imports':{'LoadLibraryA_iat_va':ll,'GetProcAddress_iat_va':gp},
      'chrmodels_load':{'path':r'SYSTEM\chrmodels.dll','path_va':chrpath,'xref_vas':loader_xrefs,'loader_function_va':0x401ac0},
      'sonic_exports':sonic,
      'runtime_witnesses':{
        'action_table_pointer_compare_va':0x49442c,
        'action_object_field_read_va':0x494426,
        'action_motion_field_read_va':0x494464,
        'sonic_motion_table_special_case_va':0x4945a8,
        'validated_bytes':runtime_bytes,
      }
    }

def analyze_modloader(path):
    pe=PE32(path)
    if len(pe.data)!=MODLOADER_SIZE or pe.sha256()!=MODLOADER_SHA256: raise ValueError('SADXModLoader.dll fingerprint mismatch')
    er,es=pe.export_dir(); imports=pe.import_iat()
    orig_utf16='CHRMODELS_orig'.encode('utf-16le')+b'\0\0'
    o=pe.data.find(orig_utf16)
    if o<0: raise ValueError('missing CHRMODELS_orig wide string')
    origva=pe.off_to_va(o)
    # Validate the exact instructions that back the promoted counts/layout.
    witness_bytes={
      'orig_probe': expect_bytes(pe,0x1005ea57,b'\x68\xe4\xbf\x0d\x10\x68\xc4\xbf\x0d\x10','CHRMODELS_orig export probe'),
      'object_model_plus_04': expect_bytes(pe,0x10093237,b'\x8b\x4e\x04','object model +0x04'),
      'object_child_plus_2c': expect_bytes(pe,0x10093243,b'\x8b\x4e\x2c','object child +0x2c'),
      'object_sibling_plus_30': expect_bytes(pe,0x1009324f,b'\x8b\x76\x30','object sibling +0x30'),
      'sonic_objects_79': expect_bytes(pe,0x100937bc,b'\x6a\x01\x6a\x4f','Sonic object count 79'),
      'sonic_actions_149': expect_bytes(pe,0x10093841,b'\x81\xfe\x95\x00\x00\x00','Sonic action count 149'),
      'sonic_models_11': expect_bytes(pe,0x10093849,b'\x6a\x00\x6a\x0b','Sonic model count 11'),
    }
    return {
      'file':{'size':len(pe.data),'sha256':pe.sha256(),'image_base':pe.image_base},
      'normal_pe_export_directory':{'rva':er,'size':es,'is_empty':er==0 and es==0},
      'imports':{
        'GetModuleHandleW_iat_va':imports.get(('kernel32.dll','GetModuleHandleW')),
        'GetProcAddress_iat_va':imports.get(('kernel32.dll','GetProcAddress')),
      },
      'original_module_name':{'text':'CHRMODELS_orig','va':origva},
      'original_export_probe_witness_va':0x1005ea57,
      'validated_witness_bytes':witness_bytes,
      'proven_sonic_table_counts':{'models':11,'objects':79,'actions':149},
      'proven_object_layout':{'model_ptr_offset':4,'child_ptr_offset':44,'sibling_ptr_offset':48,'minimum_size_bytes':52},
      'proven_action_layout':{'object_ptr_offset':0,'motion_ptr_offset':4,'minimum_size_bytes':8},
      'original_dll_required_fingerprint':{'size':ORIG_CHRMODELS_SIZE,'sha256':ORIG_CHRMODELS_SHA256},
      'notes':[
        'Counts are code-backed by this exact SADXModLoader build when it processes CHRMODELS_orig exports.',
        'MOTION table count is not proven here.',
        'Transform offsets/order/angle units are not promoted by this pass.'
      ]
    }

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('sonic_exe'); ap.add_argument('--modloader'); ap.add_argument('--json')
    a=ap.parse_args(); result={'pc2004':analyze_exe(a.sonic_exe)}
    if a.modloader: result['modloader_proxy']=analyze_modloader(a.modloader)
    text=json.dumps(result,indent=2)
    if a.json: Path(a.json).write_text(text+'\n')
    print(text)
if __name__=='__main__': main()
