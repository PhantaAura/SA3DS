#ifndef SA3DS_SET_TASK_LIFETIME_H
#define SA3DS_SET_TASK_LIFETIME_H
#include <stdint.h>
/*Pointer-free projection of the SET entry. Flags/counter survive ordinary
 *unloading; only active bit0 and task association are cleared on free.*/
typedef struct {uint32_t task;float unload_distance_squared;uint16_t flags;uint8_t loads,free_main;} SA3DSSetTaskLifetime;
int sa3ds_set_task_load_eligible(const SA3DSSetTaskLifetime *entry);
/*Call only AFTER real allocation succeeds and source load/filter checks pass.*/
int sa3ds_set_task_load_commit(SA3DSSetTaskLifetime *entry,uint32_t task,float load_distance_squared);
int sa3ds_set_task_defer_free(SA3DSSetTaskLifetime *entry,uint32_t task);
/*Caller invokes40B570 on the next scheduled callback, not in the clip check.
 *The source registration check precedes destructor. Destructor runs before
 *entry association/active bit are cleared; callback may set persistent flags.*/
int sa3ds_set_task_run_free(SA3DSSetTaskLifetime *entry,uint32_t task,int registered,
 void (*destructor)(void *context,uint32_t task),void *context,uint8_t *freed);
#endif
