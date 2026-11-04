#ifndef SA3DS_OWNED_TASK_LIST_H
#define SA3DS_OWNED_TASK_LIST_H
#include <stdint.h>
typedef struct SA3DSOwnedTaskList SA3DSOwnedTaskList;
typedef int (*SA3DSOwnedTaskCallback)(SA3DSOwnedTaskList *list,uint32_t task,void *context);
typedef struct {
 uint32_t next,previous;
 SA3DSOwnedTaskCallback main,destructor;
 void *data;
 uint8_t used,destroying;
} SA3DSOwnedTask;
struct SA3DSOwnedTaskList {
 SA3DSOwnedTask *slots;uint32_t capacity,head,free_head,current;uint8_t running;
};
/*One flat root list, caller-owned pool. No heap work per update. IDs are
 *slot identities, reused from a LIFO free list like task-pointer reuse.*/
int sa3ds_owned_tasks_init(SA3DSOwnedTaskList *list,SA3DSOwnedTask *slots,uint32_t capacity);
SA3DSOwnedTask *sa3ds_owned_task(SA3DSOwnedTaskList *list,uint32_t task);
uint32_t sa3ds_owned_task_allocate(SA3DSOwnedTaskList *list,SA3DSOwnedTaskCallback main,void *data);
int sa3ds_owned_task_set_main(SA3DSOwnedTaskList *list,uint32_t task,SA3DSOwnedTaskCallback main);
/*During traversal only release the current task; defer another task by
 *replacing main. Caller destructor handles its native resources/children.*/
int sa3ds_owned_task_release(SA3DSOwnedTaskList *list,uint32_t task,void *context);
/*40B0C0: snapshot next LINK, then dispatch the current callback. New head
 *tasks wait for another traversal; future callback replacements are observed.
 *Hierarchy, script-origin globals and recursive root traversal are separate.*/
int sa3ds_owned_tasks_run(SA3DSOwnedTaskList *list,void *context);
#endif
