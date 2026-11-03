#ifndef SA3DS_CHECKPOINT_PLAYER_H
#define SA3DS_CHECKPOINT_PLAYER_H
#include "restart_record.h"
#include "../sa1/physics/ground.h"
/*Bridge the current port's support-center/contact frame to task-foot/canonical
 * BAMS. Does not recover original task angle history/Euler branch selection.*/
int sa3ds_checkpoint_capture_player(SA3DSRestartRecord *record,const SA1PlayerState *player,
    int16_t act,const uint8_t time[3]);
int sa3ds_checkpoint_restore_player(const SA3DSRestartRecord *record,SA1PlayerState *player,
    const SA1PhysicsWorld *world,double (*sine)(void*,uint16_t),void *context);
int sa3ds_checkpoint_player_spin(const SA1PlayerState *player,uint16_t post_yaw,int16_t *spin);
#endif
