#ifndef SA3DS_RESTART_RECORD_H
#define SA3DS_RESTART_RECORD_H
#include "../sa1/physics/player_state.h"
#include <stdint.h>
/* Source44ED50/70,44EE70 record, kept independent of native task pointers.
 * Time is three raw bytes; rotation retains all three original int32 words.
 * This is session state, not a save-file or owner package representation. */
typedef struct {
    uint16_t stage_act;
    uint8_t minutes,seconds,frames;
    uint32_t valid;
    SA1Vec3 task_position;
    int32_t task_angles[3];
} SA3DSRestartRecord;
void sa3ds_restart_record_clear(SA3DSRestartRecord *record);
/*44ED70 compares the full sign-extended level/act expression against the
 * zero-extended saved word, clearing the record on a mismatch.*/
uint32_t sa3ds_restart_record_check(SA3DSRestartRecord *record,int16_t level,int16_t act);
void sa3ds_restart_record_store(SA3DSRestartRecord *record,int16_t level,int16_t act,
    SA1Vec3 task_position,const int32_t angles[3],const uint8_t time[3]);
/*Ordinary Emerald Coast44EDB0 payload. Caller performs ring reset/441FD0 and
 * source stage/player setup. The unrelated level3/act2/Gamma override is not
 * part of this ordinary payload function. Call check before restoring.*/
void sa3ds_restart_record_restore(const SA3DSRestartRecord *record,
    SA1Vec3 *task_position,int32_t angles[3],uint8_t time[3]);
#endif
