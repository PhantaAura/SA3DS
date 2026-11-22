#include "profile.h"
#include <tlhelp32.h>
#include <stdlib.h>
static uintptr_t remote_load_library(DWORD pid) {
 HMODULE local=NULL; FARPROC load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
 wchar_t path[32768];
 if(!load||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)load,&local))return 0;
 if(!GetModuleFileNameW(local,path,32768))return 0;
 const wchar_t *name=wcsrchr(path,L'\\');name=name?name+1:path;
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W entry={.dwSize=sizeof(entry)};uintptr_t result=0;
 if(Module32FirstW(snapshot,&entry))do{
  if(!_wcsicmp(entry.szModule,name)){result=(uintptr_t)entry.modBaseAddr+(uintptr_t)load-(uintptr_t)local;break;}
 }while(Module32NextW(snapshot,&entry));
 CloseHandle(snapshot);return result;
}
static DWORD unique_sonic_pid(void) {
 DWORD result=0;HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W entry={.dwSize=sizeof(entry)};
 if(Process32FirstW(snapshot,&entry))do{
  const wchar_t *name=wcsrchr(entry.szExeFile,L'\\');name=name?name+1:entry.szExeFile;
  if(!_wcsicmp(name,L"sonic.exe")){
   if(result){fprintf(stderr,"Multiple Sonic processes; refusing ambiguous target\n");result=0;break;}
   result=entry.th32ProcessID;
  }
 }while(Process32NextW(snapshot,&entry));
 CloseHandle(snapshot);return result;
}
int wmain(int argc,wchar_t **argv) {
 if(argc!=2&&(argc!=4||wcscmp(argv[2],L"--expected-game"))){fprintf(stderr,"usage: inject.exe (--probe | observer.dll) [--expected-game sonic.exe]\n");return 2;}
 DWORD pid=unique_sonic_pid();
 if(!pid){fprintf(stderr,"No unique Sonic process in this Wine prefix\n");return 2;}
 int probe=!wcscmp(argv[1],L"--probe");
 DWORD rights=PROCESS_QUERY_INFORMATION|PROCESS_VM_READ;
 if(!probe)rights|=PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE;
 HANDLE process=OpenProcess(rights,FALSE,pid);
 if(!process){fprintf(stderr,"OpenProcess failed: %lu\n",GetLastError());return 1;}
 wchar_t exe[32768];int result=1;
 if(!GetModuleFileNameExW(process,NULL,exe,32768)||!profile_file(exe)){fprintf(stderr,"Unsupported or unreadable Sonic executable\n");goto done;}
 if(argc==4){
  wchar_t expected[32768];DWORD n=GetFullPathNameW(argv[3],32768,expected,NULL);
  if(!n||n>=32768||_wcsicmp(exe,expected)){
   fprintf(stderr,"Sonic executable path differs from the requested reference copy\n");goto done;
  }
 }
 printf("Verified PC2004 process: Windows pid=%lu\n",pid);fflush(stdout);
 /* Loading code hooks while Wine is still constructing the process can
  * stall startup. An admitted executable alone is not a ready gameplay task. */
 uint32_t entity=0;SIZE_T entity_bytes=0;
 int gameplay_ready=ReadProcessMemory(process,(LPCVOID)0x03b42e10,&entity,4,&entity_bytes)&&entity_bytes==4&&entity!=0;
 printf("Sonic player task ready: %s\n",gameplay_ready?"yes":"no");
 if(!probe&&!gameplay_ready){fprintf(stderr,"Wait for Sonic gameplay before loading the observer\n");goto done;}
 if(probe){uint32_t peripheral=0;SIZE_T n=0;unsigned char raw[0x34];
  if(!ReadProcessMemory(process,(LPCVOID)0x03b0e77c,&peripheral,4,&n)||n!=4||!peripheral||
     !ReadProcessMemory(process,(LPCVOID)(uintptr_t)peripheral,raw,sizeof(raw),&n)||n!=sizeof(raw)){
   fprintf(stderr,"Controller read failed: %lu\n",GetLastError());goto done;}
  uint32_t held,pressed;int16_t x,y;memcpy(&held,raw+8,4);memcpy(&pressed,raw+0x10,4);memcpy(&x,raw+0x1c,2);memcpy(&y,raw+0x1e,2);
  printf("Game-only input probe: held=%08lx pressed=%08lx stick=%d,%d\n",(unsigned long)held,(unsigned long)pressed,x,y);result=0;goto done;
 }
 DWORD length=GetFullPathNameW(argv[1],32768,exe,NULL);
 if(!length||length>=32768||GetFileAttributesW(exe)==INVALID_FILE_ATTRIBUTES){fprintf(stderr,"Observer DLL path unavailable\n");goto done;}
 uintptr_t load=remote_load_library(pid);
 if(!load){fprintf(stderr,"Cannot resolve target LoadLibraryW\n");goto done;}
 SIZE_T bytes=(wcslen(exe)+1)*sizeof(wchar_t),written=0;
 LPVOID argument=VirtualAllocEx(process,NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!argument||!WriteProcessMemory(process,argument,exe,bytes,&written)||written!=bytes){fprintf(stderr,"Cannot prepare DLL path\n");if(argument)VirtualFreeEx(process,argument,0,MEM_RELEASE);goto done;}
 HANDLE thread=CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)load,argument,0,NULL);
 if(!thread){fprintf(stderr,"Cannot load observer: %lu\n",GetLastError());VirtualFreeEx(process,argument,0,MEM_RELEASE);goto done;}
 if(WaitForSingleObject(thread,15000)!=WAIT_OBJECT_0){fprintf(stderr,"Observer load timed out; leaving its pending argument allocated\n");CloseHandle(thread);goto done;}
 DWORD module=0;GetExitCodeThread(thread,&module);CloseHandle(thread);VirtualFreeEx(process,argument,0,MEM_RELEASE);
 if(!module){fprintf(stderr,"Observer DLL load failed\n");goto done;}
 printf("Observer loaded at %08lx; inspect its ready/result files for capture acceptance\n",module);result=0;
 done:CloseHandle(process);return result;
}
