#include "set_task_lifetime.h"
#include "set_object_clip.h"
int sa3ds_set_task_load_eligible(const SA3DSSetTaskLifetime*e){return e&&(e->flags&0x8000)&&!(e->flags&1);}
int sa3ds_set_task_load_commit(SA3DSSetTaskLifetime*e,uint32_t task,float load){
 float unload;if(!e||!task||e->task||!sa3ds_set_task_load_eligible(e)||!sa3ds_set_unload_distance(load,&unload))return 0;
 e->flags|=1;e->loads++;e->task=task;e->unload_distance_squared=unload;e->free_main=0;return 1;
}
int sa3ds_set_task_defer_free(SA3DSSetTaskLifetime*e,uint32_t task){
 if(!e||!task||e->task!=task)return 0;
 e->free_main=1;return 1;
}
int sa3ds_set_task_run_free(SA3DSSetTaskLifetime*e,uint32_t task,int registered,
 void(*destructor)(void*,uint32_t),void*context,uint8_t*freed){
 if(!e||!task||e->task!=task||!e->free_main||!freed)return 0;
 *freed=0;if(!registered)return 1;
 if(destructor)destructor(context,task);
 e->task=0;e->flags&=(uint16_t)~1u;e->free_main=0;*freed=1;return 1;
}
