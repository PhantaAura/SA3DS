#ifndef SA3DS_SET_PREPARATION_H
#define SA3DS_SET_PREPARATION_H
#include "set_task_lifetime.h"
/*46C1D0 zeroes every SET work entry, then46BD20 appends the act's rows: flags
 *|=0x8000, load counter0. A row whose (type&FFF)+(act*100+level)*100 equals
 *global0091BA00 also receives a mission-table flag word; that table is not
 *imported, so a match is rejected instead of guessed. The exact executable has
 *no static writer of0091BA00 (zero-filled), which cannot match a level>=1 row.*/
int sa3ds_set_cold_prepare(SA3DSSetTaskLifetime *entries,const uint16_t *types,uint32_t count,
 uint8_t level,uint8_t act,int32_t mission_object);
/*46BE00 act-change clear. Entries without0x4000 keep only flags&7FE9; an
 *entry whose OLD flags had active bit0 gets main40B570 (deferred free). Bit0
 *and the task association survive until that callback runs.*/
int sa3ds_set_act_clear(SA3DSSetTaskLifetime *entries,uint32_t count,
 int (*request_free)(void *context,uint32_t task),void *context);
/*46BE50 compaction for this fixed-row projection: unpreserved entries are
 *zeroed (restored payload release, flag2, is rejected as unsupported);
 *preserved entries clear0x4000 and keep their row identity.*/
int sa3ds_set_act_compact(SA3DSSetTaskLifetime *entries,uint32_t count);
/*46BF20 (from49DD70): entries without0x10 become flags0x8000/loads0.*/
int sa3ds_set_restart_reset(SA3DSSetTaskLifetime *entries,uint32_t count);
#endif
