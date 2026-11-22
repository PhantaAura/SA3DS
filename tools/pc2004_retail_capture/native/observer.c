#include "profile.h"
#include "capture_format.h"
#include "MinHook.h"
#include <wchar.h>
#include <stdlib.h>

typedef struct { unsigned char bytes[512]; } __attribute__((aligned(16))) FpState;
static inline void fp_save(FpState *state){__asm__ __volatile__("fxsave %0":"=m"(*state)::"memory");}
static inline void fp_restore(FpState *state){__asm__ __volatile__("fxrstor %0"::"m"(*state):"memory");}
typedef struct {
 uint32_t sequence,thread,angle,camera,held,pressed,released,camera_valid;
 int32_t x,y;float magnitude;
} InputSnapshot;
static HMODULE instance;
static wchar_t directory[32768];
static void (__cdecl *original_filter)(void);
static void (__cdecl *original_main)(void *);
static void (__cdecl *original_act1)(void *,uintptr_t,void *);
static int (__cdecl *original_ground)(void *,uintptr_t,void *);
static InputSnapshot latest_input;
static CaptureRecord *records;
/* Separate sidecar preserves the v1/v2 playback trace layout. */
typedef struct {
 uint32_t frame;
 float distance_before,distance_after;
 uint32_t head_before,head_after;
 int32_t control_timer_before;
 uint32_t task_yaw_before,motion_yaw_before;
 float position_before[3],local_velocity_before[3],world_velocity_before[3];
 uint32_t task_angles_before[3];
 float collision_up_before[3];
} LoopRecord;
_Static_assert(sizeof(LoopRecord)==92,"loop sidecar layout");
static LoopRecord *loop_records;
/* Optional collision-only sidecar; primary animation/input records stay stable. */
typedef struct {
 uint32_t frame,calls,angles[3];
 float position[3],motion[3],local[3];
 uint32_t status,terrain_flags;
 float height,pos_error,run_speed,jog_speed,dash_speed;
 unsigned char collision_after[132];
} GroundRecord;
_Static_assert(sizeof(GroundRecord)==216,"ground sidecar layout");
static GroundRecord *ground_records,ground_current;
typedef struct {uint32_t frame,count;unsigned char entries[16*156];} GroundHits;
_Static_assert(sizeof(GroundHits)==2504,"ground hit sidecar layout");
static GroundHits *ground_hits,ground_hits_current;
static int ground_enabled;
static CaptureHeader header;
static volatile LONG active_callbacks,recording,row_count,act1_rows,errors,warmup_skipped;
static uint32_t maximum_rows,active_thread,act1_sequence;
static uintptr_t active_entity;
static int copy_memory(uintptr_t address,void *target,size_t size){
 SIZE_T got=0;return address&&ReadProcessMemory(GetCurrentProcess(),(LPCVOID)address,target,size,&got)&&got==size;
}
static uint32_t u32(const void *bytes,size_t offset){uint32_t value;memcpy(&value,(const unsigned char*)bytes+offset,4);return value;}
static uint16_t u16(const void *bytes,size_t offset){uint16_t value;memcpy(&value,(const unsigned char*)bytes+offset,2);return value;}
static int16_t s16(const void *bytes,size_t offset){return (int16_t)u16(bytes,offset);}
static float f32(const void *bytes,size_t offset){float value;memcpy(&value,(const unsigned char*)bytes+offset,4);return value;}
static int path_for(wchar_t *out,const wchar_t *name){int count=swprintf(out,32768,L"%ls\\%ls",directory,name);return count>0&&count<32768;}
static int write_new(const wchar_t *name,const void *data,DWORD length){
 wchar_t path[32768];if(!path_for(path,name))return 0;
 HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE)return 0;
 DWORD written=0;int ok=WriteFile(file,data,length,&written,NULL)&&written==length;CloseHandle(file);return ok;
}
static void fail(const char *message){InterlockedOr(&errors,(LONG)0x80000000u);write_new(L"capture.error.txt",message,(DWORD)strlen(message));}
static void __cdecl filter_hook(void){
 InterlockedIncrement(&active_callbacks);original_filter();FpState fp;fp_save(&fp);
 uint32_t peripheral=0,camera=0;unsigned char pad[0x34];InputSnapshot input={0};
 if(copy_memory(0x03b0e77c,&peripheral,4)&&copy_memory(peripheral,pad,sizeof(pad))){
  input.x=s16(pad,0x1c);input.y=s16(pad,0x1e);input.held=u32(pad,8);input.pressed=u32(pad,0x10);input.released=u32(pad,0x14);
  if(copy_memory(0x03b2cbb0,&camera,4)&&camera)input.camera_valid=copy_memory(camera+0x18,&input.camera,4);
  input.thread=GetCurrentThreadId();input.sequence=latest_input.sequence+1;latest_input=input;
 }
 fp_restore(&fp);InterlockedDecrement(&active_callbacks);
}
static void __cdecl act1_hook(void *entity,uintptr_t motion,void *player){
 InterlockedIncrement(&active_callbacks);
 if((uintptr_t)entity==active_entity&&GetCurrentThreadId()==active_thread)act1_sequence++;
 original_act1(entity,motion,player);InterlockedDecrement(&active_callbacks);
}
static int __cdecl ground_hook(void *entity,uintptr_t motion,void *player){
 InterlockedIncrement(&active_callbacks);FpState before;fp_save(&before);
 int selected=ground_enabled&&(uintptr_t)entity==active_entity&&GetCurrentThreadId()==active_thread;
 uint32_t csts=0;
 if(selected){
  GroundRecord *r=&ground_current;unsigned char e[0x2c],p[0x134];r->calls++;
  if(r->calls!=1||!copy_memory((uintptr_t)entity,e,sizeof(e))||
     !copy_memory((uintptr_t)player,p,sizeof(p))||!copy_memory(motion+4,r->motion,12))
   InterlockedOr(&errors,32);
  else{
   memcpy(r->angles,e+0x14,12);memcpy(r->position,e+0x20,12);memcpy(r->local,p+0x38,12);
   r->status=u16(e,4);r->terrain_flags=u32(p,0x60);r->height=f32(p,0x110);r->pos_error=f32(p,0xa0);
   r->jog_speed=f32(p,0xc0);r->run_speed=f32(p,0xc4);r->dash_speed=f32(p,0xd0);csts=u32(p,0x64);
  }
 }
 fp_restore(&before);int result=original_ground(entity,motion,player);FpState after;fp_save(&after);
 if(selected){
  if(!csts||!copy_memory(csts,ground_current.collision_after,132)||
     !copy_memory(0x03b42ff4,&ground_hits_current.count,4)||ground_hits_current.count>16||
     (ground_hits_current.count&&!copy_memory(0x03b47590,ground_hits_current.entries,ground_hits_current.count*156)))InterlockedOr(&errors,32);
 }
 fp_restore(&after);InterlockedDecrement(&active_callbacks);return result;
}
static void __cdecl main_hook(void *task){
 InterlockedIncrement(&active_callbacks);FpState before;fp_save(&before);
 InputSnapshot input=latest_input;uint32_t pointers[2]={0};unsigned char entity_head[12];
 unsigned char logical[16]={0};
 int selected=InterlockedCompareExchange(&recording,0,0)&&copy_memory((uintptr_t)task+0x20,pointers,8)&&
  copy_memory(pointers[0],entity_head,sizeof(entity_head))&&entity_head[8]==0;
 if(selected&&!input.sequence){InterlockedIncrement(&warmup_skipped);selected=0;}
 if(selected&&(input.thread!=GetCurrentThreadId()||active_entity)){InterlockedOr(&errors,1);selected=0;}
 /* 43BDC0 consumes 3B0E7A0/4. The earlier 40F170 snapshot at3B0E368
    is the unused normalized buffer, not Sonic's active logical analog.
    Read the active buffer at main entry, after input processing finishes. */
 unsigned char active_analog[8];
 if(selected){
  if(!copy_memory(0x03b0e7a0,active_analog,sizeof(active_analog))){InterlockedOr(&errors,8);selected=0;}
  else{input.angle=u32(active_analog,0);input.magnitude=f32(active_analog,4);}
 }
 if(selected&&!copy_memory(0x03b0e7f8,logical,sizeof(logical))){InterlockedOr(&errors,4);selected=0;}
 LoopRecord loop_before={0};
 if(selected){
  uint32_t player=0,csts=0;int16_t timer=0;
  int ok=copy_memory(pointers[1],&player,4)&&player&&
    copy_memory(player+0x18,&loop_before.distance_before,4)&&
    copy_memory(player+0x0a,&timer,2)&&
    copy_memory(pointers[0]+0x10,&loop_before.head_before,4)&&
    copy_memory(pointers[0]+0x18,&loop_before.task_yaw_before,4)&&
    copy_memory(pointers[1]+0x20,&loop_before.motion_yaw_before,4)&&
    copy_memory(pointers[0]+0x20,loop_before.position_before,12)&&
    copy_memory(pointers[0]+0x14,loop_before.task_angles_before,12)&&
    copy_memory(player+0x38,loop_before.local_velocity_before,12)&&
    copy_memory(pointers[1]+4,loop_before.world_velocity_before,12)&&
    copy_memory(player+0x64,&csts,4)&&csts&&copy_memory(csts+0x1c,loop_before.collision_up_before,12);
  loop_before.control_timer_before=timer;
  if(!ok){InterlockedOr(&errors,16);selected=0;}
 }
 uint32_t act_before=act1_sequence;uintptr_t prior_entity=active_entity;uint32_t prior_thread=active_thread;
 if(selected){active_entity=pointers[0];active_thread=GetCurrentThreadId();memset(&ground_current,0,sizeof(ground_current));memset(&ground_hits_current,0,sizeof(ground_hits_current));}
 fp_restore(&before);original_main(task);FpState after;fp_save(&after);
 active_entity=prior_entity;active_thread=prior_thread;
 if(selected&&InterlockedCompareExchange(&recording,0,0)){
  LONG index=InterlockedCompareExchange(&row_count,0,0);unsigned char e[0x2c],m[0x10],p[0x134],collision[0x54];
  if((uint32_t)index<maximum_rows){
   if(copy_memory(pointers[0],e,sizeof(e))&&copy_memory(pointers[1],m,sizeof(m))&&copy_memory(u32(m,0),p,sizeof(p))){
    CaptureRecord *r=&records[index];memset(r,0,sizeof(*r));
    if(ground_enabled){ground_records[index]=ground_current;ground_records[index].frame=(uint32_t)index;
     ground_hits[index]=ground_hits_current;ground_hits[index].frame=(uint32_t)index;}
    loop_records[index]=loop_before;loop_records[index].frame=(uint32_t)index;
    loop_records[index].distance_after=f32(p,0x18);loop_records[index].head_after=u32(e,0x10);
    r->frame=(uint32_t)index;r->input_sequence=input.sequence;r->act1_ran=act1_sequence>act_before;
    r->raw_x=input.x;r->raw_y=input.y;r->filtered_magnitude=input.magnitude;r->filtered_angle=input.angle;
    r->camera_yaw=input.camera;r->camera_valid=input.camera_valid;
    r->logical_held=u32(logical,0);r->logical_pressed=u32(logical,8);r->logical_released=u32(logical,12);r->held=input.held;r->pressed=input.pressed;r->released=input.released;
    for(unsigned axis=0;axis<3;axis++){
     r->position[axis]=f32(e,0x20+4*axis);r->world_velocity[axis]=f32(m,4+4*axis);
     r->local_velocity[axis]=f32(p,0x38+4*axis);r->task_angles[axis]=u32(e,0x14+4*axis);
     r->acceleration[axis]=f32(p,0x2c+4*axis);
    }
    r->status=u16(e,4);r->action=e[0];r->player_bits=u32(p,0x5c);r->player_low_status=u16(p,0x10);
    r->terrain_flags=u32(p,0x60);r->support_model=u32(p,0x74);r->spindash_charge=f32(p,0);
    r->spindash_ticks=u16(p,0x7e);r->animation_requested=s16(p,0x124);r->animation_source=s16(p,0x126);
    r->animation_transition=s16(p,0x120);r->animation_frame=f32(p,0x130);
    uint32_t csts=u32(p,0x64);
    if(csts&&copy_memory(csts,collision,sizeof(collision))){
     for(unsigned axis=0;axis<3;axis++){
      r->collision_up[axis]=f32(collision,0x1c+4*axis);r->collision_normal[axis]=f32(collision,0x3c+4*axis);r->collision_normal2[axis]=f32(collision,0x48+4*axis);
     }
     r->collision_valid=1;r->collision_count=u16(collision,0x28);r->collision_selected=u16(collision,0x2a);
    }
    LARGE_INTEGER now;QueryPerformanceCounter(&now);r->qpc=(uint64_t)now.QuadPart;
    if(r->act1_ran)InterlockedIncrement(&act1_rows);
    InterlockedExchange(&row_count,index+1);
   }else InterlockedOr(&errors,2);
  }
 }
 fp_restore(&after);InterlockedDecrement(&active_callbacks);
}
static int original_entries_match(const wchar_t *path){
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE)return 0;
 HANDLE mapping=CreateFileMappingW(file,NULL,PAGE_READONLY,0,0,NULL);CloseHandle(file);if(!mapping)return 0;
 unsigned char *data=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);int ok=data!=NULL;
 if(data){
  IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)data;IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32*)(data+dos->e_lfanew);
  IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);const uint32_t addresses[]={0x0040f170,0x00496f50,0x0049a9b0,0x00449500};
  header.image_size=nt->OptionalHeader.SizeOfImage;
  for(unsigned item=0;item<4;item++){
   uint32_t rva=addresses[item]-0x400000;int found=0;
   for(unsigned i=0;i<nt->FileHeader.NumberOfSections;i++)if(rva>=sections[i].VirtualAddress&&rva+32<=sections[i].VirtualAddress+sections[i].SizeOfRawData){
    const unsigned char *disk=data+sections[i].PointerToRawData+rva-sections[i].VirtualAddress;
    found=!memcmp(disk,(const void*)(uintptr_t)addresses[item],32);break;
   }
   if(!found)ok=0;
  }
  UnmapViewOfFile(data);
 }
 CloseHandle(mapping);return ok;
}
static DWORD WINAPI capture_worker(void *unused){
 (void)unused;wchar_t executable[32768],config[32768];int initialized=0,enabled=0;
 if((uintptr_t)GetModuleHandleW(NULL)!=0x400000||!GetModuleFileNameW(NULL,executable,32768)||!profile_file(executable)){
  fail("Unsupported PC2004 image identity\n");return 1;
 }
 memset(&header,0,sizeof(header));memcpy(header.magic,"SA3DPCTR",8);header.version=2;
 header.header_bytes=sizeof(header);header.record_bytes=sizeof(CaptureRecord);header.image_base=0x400000;
 memcpy(header.executable_sha256,PROFILE_SHA,64);
 if(!original_entries_match(executable)){fail("Original observation entries already changed; hooks not installed\n");return 1;}
 path_for(config,L"capture.ini");maximum_rows=GetPrivateProfileIntW(L"capture",L"frames",600,config);
 if(maximum_rows<1||maximum_rows>60000){fail("Invalid requested frame count\n");return 1;}
 UINT seconds=GetPrivateProfileIntW(L"capture",L"timeout_seconds",180,config);
 if(seconds<1||seconds>1800){fail("Invalid capture timeout\n");return 1;}
 ground_enabled=GetPrivateProfileIntW(L"capture",L"ground_state",0,config)!=0;
 if(ground_enabled&&maximum_rows>6000){fail("Ground recording limit is6000 updates\n");return 1;}
 records=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)maximum_rows*sizeof(*records));
 loop_records=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)maximum_rows*sizeof(*loop_records));
 if(ground_enabled)ground_records=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)maximum_rows*sizeof(*ground_records));
 if(ground_enabled)ground_hits=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)maximum_rows*sizeof(*ground_hits));
 if(!records||!loop_records||(ground_enabled&&(!ground_records||!ground_hits))){
  if(records)HeapFree(GetProcessHeap(),0,records);
  if(loop_records)HeapFree(GetProcessHeap(),0,loop_records);
  if(ground_records)HeapFree(GetProcessHeap(),0,ground_records);
  if(ground_hits)HeapFree(GetProcessHeap(),0,ground_hits);
  fail("Capture allocation failed\n");return 1;
 }
 if(MH_Initialize()!=MH_OK){fail("MinHook initialization failed\n");goto done;}initialized=1;
 if(MH_CreateHook((void*)0x0040f170,filter_hook,(void**)&original_filter)!=MH_OK||
    MH_CreateHook((void*)0x00496f50,act1_hook,(void**)&original_act1)!=MH_OK||
    MH_CreateHook((void*)0x0049a9b0,main_hook,(void**)&original_main)!=MH_OK){fail("Observation hook creation failed\n");goto done;}
 if(ground_enabled&&MH_CreateHook((void*)0x00449500,ground_hook,(void**)&original_ground)!=MH_OK){fail("Ground observation hook creation failed\n");goto done;}
 if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK){fail("Observation hook activation failed\n");goto done;}enabled=1;
 LARGE_INTEGER start,frequency;QueryPerformanceCounter(&start);QueryPerformanceFrequency(&frequency);
 header.qpc_start=(uint64_t)start.QuadPart;header.qpc_frequency=(uint64_t)frequency.QuadPart;header.hook_mask=ground_enabled?15:7;
 InterlockedExchange(&recording,1);
 char ready[256];snprintf(ready,sizeof(ready),"{\"phase\":\"post_sonic_main\",\"hooks\":%u,\"writes_gameplay_state\":false,\"normalized_input_source\":\"active_at_sonic_main_entry\"}\n",ground_enabled?4u:3u);
 if(!write_new(L"capture.ready.json",ready,(DWORD)strlen(ready))){
  fail("Cannot publish capture readiness\n");goto done;
 }
 if(ground_enabled){const char note[]="{\"phase\":\"449500_entry_and_exit\",\"record_bytes\":216,\"hit_record_bytes\":2504}\n";
  if(!write_new(L"ground.ready.json",note,sizeof(note)-1)){fail("Cannot publish ground readiness\n");goto done;}
 }
 ULONGLONG deadline=GetTickCount64()+(ULONGLONG)seconds*1000;
 while((uint32_t)InterlockedCompareExchange(&row_count,0,0)<maximum_rows&&GetTickCount64()<deadline)Sleep(20);
 done:
 InterlockedExchange(&recording,0);
 if(initialized){
  MH_STATUS stopped=MH_DisableHook(MH_ALL_HOOKS);
  if(stopped!=MH_OK&&stopped!=MH_ERROR_DISABLED)fail("Cannot disable observation hooks\n");
 }
 while(InterlockedCompareExchange(&active_callbacks,0,0))Sleep(1);
 if(enabled&&!original_entries_match(executable))fail("Original observation entries were not restored\n");
 /* Retain disabled trampolines as well as the module. A suspended thread
    could be in a detour prologue before incrementing active_callbacks. */
 if(enabled){
  header.record_count=(uint32_t)row_count;header.act1_rows=(uint32_t)act1_rows;header.errors=(uint32_t)errors;header.warmup_skipped=(uint32_t)warmup_skipped;
  if(!write_new(L"loop-state.raw",loop_records,header.record_count*sizeof(*loop_records)))
   fail("Cannot create loop sidecar\n");
  if(ground_enabled&&!write_new(L"ground-state.raw",ground_records,header.record_count*sizeof(*ground_records)))
   fail("Cannot create ground sidecar\n");
  if(ground_enabled&&!write_new(L"ground-hits.raw",ground_hits,header.record_count*sizeof(*ground_hits)))
   fail("Cannot create ground hit sidecar\n");
  wchar_t output[32768];path_for(output,L"capture.raw");
  HANDLE file=CreateFileW(output,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  if(file!=INVALID_HANDLE_VALUE){DWORD written=0;DWORD bytes=header.record_count*sizeof(*records);
   if(!WriteFile(file,&header,sizeof(header),&written,NULL)||written!=sizeof(header)||
      !WriteFile(file,records,bytes,&written,NULL)||written!=bytes)fail("Capture file write failed\n");
   CloseHandle(file);
  }else fail("Cannot create capture result\n");
 }
 HeapFree(GetProcessHeap(),0,records);records=NULL;
 HeapFree(GetProcessHeap(),0,loop_records);loop_records=NULL;
 if(ground_records)HeapFree(GetProcessHeap(),0,ground_records);ground_records=NULL;
 if(ground_hits)HeapFree(GetProcessHeap(),0,ground_hits);ground_hits=NULL;
 /* Keep the disabled module loaded: unloading its code while a detour's
    epilogue returns would race the game thread. No hooks remain enabled. */
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved){
 (void)reserved;if(reason==DLL_PROCESS_ATTACH){instance=module;DisableThreadLibraryCalls(module);
  DWORD count=GetModuleFileNameW(module,directory,32768);if(!count||count>=32768)return FALSE;
  wchar_t *slash=wcsrchr(directory,L'\\');if(!slash)return FALSE;*slash=0;
  HANDLE worker=CreateThread(NULL,0,capture_worker,NULL,0,NULL);if(!worker)return FALSE;CloseHandle(worker);
 }return TRUE;
}
