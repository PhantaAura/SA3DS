#ifndef SA3DS_CHECKPOINT_CONTACT_H
#define SA3DS_CHECKPOINT_CONTACT_H
#include "ccl_contact.h"
typedef struct {
    SA1Vec3 previous_task,current_task,box_center;
    float player_radius,player_half_height,player_foot_radius;
    float box_x,box_y,box_z;
    uint16_t box_yaw;
    double (*sine)(void *context,uint16_t angle);
    void *context;
} SA3DSCheckpointSweep;
/*41FD00 form7->3 routes419C90, which tests4199D0's expanded yaw rectangle.
 * Result kind:0 miss,1 top,2 side,3 underside. No invented closest-point sphere
 * or full-tilt box test. Caller retains source broad/category/CCL scheduling.*/
typedef struct {uint8_t kind;uint32_t samples,sample_index; /*planned source sample count; index is first hit*/SA1Vec3 sample;} SA3DSCheckpointContact;
int sa3ds_checkpoint_sweep(const SA3DSCheckpointSweep *input,SA3DSCheckpointContact *out);
/*Supported checkpoint profile has source push masks inhibiting displacement.
 * Reject an enabled push instead of dropping the original response silently.*/
int sa3ds_checkpoint_record_contact(SA3DSCCLBody *player,SA3DSCCLShape *primary,
    SA3DSCCLBody *checkpoint,SA3DSCCLShape *box,const SA3DSCheckpointContact *contact);
#endif
