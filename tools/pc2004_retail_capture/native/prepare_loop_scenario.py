#!/usr/bin/env python3
"""Build a private owner-row loop scenario; no game data is written into Git.

The reference process boots through its original Trial dispatcher, then the
observer sets one owner SET start and logical forward. Movement code is unchanged.
This is an isolated scenario, not a normal-start or full-course acceptance run.
"""
from pathlib import Path
import argparse,json,os,shutil,subprocess,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[3]))
from tools.verify_pc2004_ec_route import verified_reader
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.importers.pc2004_set import read_set_source

START=r'''
#include "profile.h"
#include <wchar.h>
int wmain(int argc,wchar_t **argv){
 if(argc!=2||!profile_file(argv[1]))return 2;
 wchar_t command[32768];
 if(swprintf(command,32768,L"\"%ls\"",argv[1])<0)return 2;
 STARTUPINFOW si={0};PROCESS_INFORMATION pi={0};si.cb=sizeof(si);
 if(!CreateProcessW(argv[1],command,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,NULL,&si,&pi))return 3;
 unsigned char original=0,trial=9;SIZE_T count=0;
 uint16_t level=1,character=0;uint32_t act=0;
 int ok=ReadProcessMemory(pi.hProcess,(void*)0x40c10c,&original,1,&count)&&count==1&&original==ORIGINAL_BOOT_BYTE;
 if(ok)ok=WriteProcessMemory(pi.hProcess,(void*)0x40c10c,&trial,1,&count)&&count==1;
 if(ok)ok=WriteProcessMemory(pi.hProcess,(void*)0x3b22dcc,&level,2,&count)&&count==2;
 if(ok)ok=WriteProcessMemory(pi.hProcess,(void*)0x3b22dc0,&character,2,&count)&&count==2;
 if(ok)ok=WriteProcessMemory(pi.hProcess,(void*)0x3b22dec,&act,4,&count)&&count==4;
 if(ok)ok=ResumeThread(pi.hThread)!=(DWORD)-1;
 if(!ok)TerminateProcess(pi.hProcess,1);
 else printf("Started private Emerald Coast reference PID %lu\n",pi.dwProcessId);
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return ok?0:4;
}
'''
RAW_CLEAR=r'''
 uint32_t physical=0;
 if(copy_memory(0x03b0e77c,&physical,4)&&physical){
  uint32_t zero=0,notheld=0xffffffffu;
  memcpy((void*)(uintptr_t)(physical+8),&zero,4);memcpy((void*)(uintptr_t)(physical+12),&notheld,4);
  memcpy((void*)(uintptr_t)(physical+16),&zero,4);memcpy((void*)(uintptr_t)(physical+20),&zero,4);
  memcpy((void*)(uintptr_t)(physical+28),&zero,4);
 }
'''
ACTIVE=r'''
 uint32_t analog[2]={0,0},buttons[4]={0,0xffffffffu,0,0};
 float magnitude=.9998980760574341f;memcpy(analog+1,&magnitude,4);
 memcpy((void*)0x03b0e7a0,analog,sizeof(analog));memcpy((void*)0x03b0e7f8,buttons,sizeof(buttons));
'''
SETUP=r'''
 uint16_t level=0;uint32_t act=99;
 copy_memory(0x03b22dcc,&level,2);copy_memory(0x03b22dec,&act,4);
 if(selected&&!scenario_setup&&level==1&&act==0&&entity_head[0]!=0){
  const float position[3]={POSITION};
  uint32_t angles[3]={0,RAW_YAW,0};float zero[3]={0};uint32_t player=0;
  if(copy_memory(pointers[1],&player,4)&&player){
   memcpy((void*)(uintptr_t)(pointers[0]+0x20),position,12);
   memcpy((void*)(uintptr_t)(pointers[0]+0x14),angles,12);
   memcpy((void*)(uintptr_t)(pointers[1]+4),zero,12);
   memcpy((void*)(uintptr_t)(player+0x38),zero,12);
   scenario_setup=1;
   const char note[]="{\"setup\":\"owner row0 and logical forward; movement/animation code unchanged\"}\n";
   write_new(L"scenario.setup.json",note,sizeof(note)-1);
  }
 }
 if(!scenario_setup)selected=0;
'''
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--owner-game',type=Path,required=True)
 p.add_argument('--zig',type=Path,required=True);p.add_argument('--minhook-source',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();source=Path(__file__).resolve().parent;repo=source.parents[2];out=a.output.resolve()
 if out==repo or repo in out.parents:p.error('scenario output must be outside source checkout')
 read=verified_reader(a.owner_game/'sonic.exe')
 row=read_set_source('SET0100S.BIN',read_source(a.owner_game/'system','SET0100S.BIN')).records[0]
 if row.type_word&4095!=3:raise ValueError('expected owner row0 dash panel')
 out.mkdir(parents=True,exist_ok=False)
 for name in ('profile.h','capture_format.h'):shutil.copy2(source/name,out/name)
 text=(source/'observer.c').read_text()
 text=text.replace('static CaptureRecord *records;', 'static int scenario_setup;\nstatic CaptureRecord *records;')
 text=text.replace('InterlockedIncrement(&active_callbacks);original_filter();', 'InterlockedIncrement(&active_callbacks);'+RAW_CLEAR+'original_filter();',1)
 text=text.replace(' InputSnapshot input=latest_input;',ACTIVE+' InputSnapshot input=latest_input;',1)
 setup=SETUP.replace('POSITION',','.join(repr(v)+'f' for v in row.position)).replace('RAW_YAW',str((0x4000-row.angle_words[1])&65535))
 text=text.replace(' LoopRecord loop_before={0};',setup+' LoopRecord loop_before={0};',1)
 text=text.replace('\\"writes_gameplay_state\\":false','\\"writes_gameplay_state\\":true')
 (out/'scenario_observer.c').write_text(text)
 (out/'start.c').write_text(START.replace('ORIGINAL_BOOT_BYTE',str(read(0x40c10c,1)[0])))
 (out/'capture.ini').write_text('[capture]\nframes=360\ntimeout_seconds=60\n')
 hook=a.minhook_source.resolve();env=os.environ.copy()
 env.update(ZIG_GLOBAL_CACHE_DIR=str(out.parent/'zig-global'),ZIG_LOCAL_CACHE_DIR=str(out.parent/'zig-local'))
 cc=[str(a.zig.resolve()),'cc','-target','x86-windows-gnu','-msse2','-O2','-Wall','-Wextra','-Werror']
 subprocess.run(cc+['-municode',str(out/'start.c'),'-lbcrypt','-lpsapi','-o',str(out/'start.exe')],check=True,env=env)
 subprocess.run(cc+['-Wno-unused-parameter','-shared','-I'+str(hook/'include'),str(out/'scenario_observer.c')]+
  [str(hook/'src'/name) for name in ('buffer.c','hook.c','trampoline.c','hde/hde32.c')]+['-lbcrypt','-lpsapi','-o',str(out/'scenario_observer.dll')],check=True,env=env)
 (out/'scenario.json').write_text(json.dumps({'writes_gameplay_state':True,'source_row':0,'logical_forward':True,
  'limitations':['One-time owner placement, not natural course start.','No movement or animation code patch.','No OS keyboard/mouse injection.']},indent=2)+'\n')
 print(out)
if __name__=='__main__':main()
