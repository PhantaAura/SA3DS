#include "owned_task_list.h"
#include <stddef.h>
#include <string.h>
int sa3ds_owned_tasks_init(SA3DSOwnedTaskList*l,SA3DSOwnedTask*slots,uint32_t capacity){
 if(!l||!slots||!capacity||capacity>UINT32_MAX/sizeof(*slots))return 0;
 memset(l,0,sizeof(*l));memset(slots,0,(size_t)capacity*sizeof(*slots));l->slots=slots;l->capacity=capacity;l->free_head=1;
 for(uint32_t i=0;i<capacity;i++)slots[i].next=i+1<capacity?i+2:0;
 return 1;
}
SA3DSOwnedTask*sa3ds_owned_task(SA3DSOwnedTaskList*l,uint32_t task){
 return l&&l->slots&&task&&task<=l->capacity&&l->slots[task-1].used?&l->slots[task-1]:NULL;
}
uint32_t sa3ds_owned_task_allocate(SA3DSOwnedTaskList*l,SA3DSOwnedTaskCallback main,void*data){
 if(!l||!l->slots||!main||!l->free_head)return 0;
 uint32_t id=l->free_head;if(id>l->capacity)return 0;
 SA3DSOwnedTask*t=&l->slots[id-1];if(t->used)return 0;
 l->free_head=t->next;memset(t,0,sizeof(*t));t->used=1;t->main=main;t->data=data;t->next=l->head;
 if(l->head)l->slots[l->head-1].previous=id;
 l->head=id;return id;
}
int sa3ds_owned_task_set_main(SA3DSOwnedTaskList*l,uint32_t id,SA3DSOwnedTaskCallback main){
 SA3DSOwnedTask*t=sa3ds_owned_task(l,id);if(!t)return 0;t->main=main;return 1;
}
int sa3ds_owned_task_release(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 SA3DSOwnedTask*t=sa3ds_owned_task(l,id);
 if(!t||t->destroying||(l->running&&l->current!=id))return 0;
 t->destroying=1;
 if(t->destructor&&!t->destructor(l,id,context)){t->destroying=0;return 0;}
 /*Read live links AFTER destructor: it may prepend another task.*/
 uint32_t next=t->next,previous=t->previous;
 t->main=t->destructor=NULL;t->data=NULL;
 if(previous)l->slots[previous-1].next=next;else l->head=next;
 if(next)l->slots[next-1].previous=previous;
 t->used=t->destroying=0;t->previous=0;t->next=l->free_head;l->free_head=id;return 1;
}
int sa3ds_owned_tasks_run(SA3DSOwnedTaskList*l,void*context){
 if(!l||!l->slots||l->running)return 0;
 l->running=1;uint32_t id=l->head;
 while(id){
  SA3DSOwnedTask*t=sa3ds_owned_task(l,id);if(!t||t->next==id)goto fail;
  uint32_t next=t->next;SA3DSOwnedTaskCallback main=t->main;l->current=id;
  if(main&&!main(l,id,context))goto fail;
  id=next;
 }
 l->current=0;l->running=0;return 1;
fail:l->current=0;l->running=0;return 0;
}
