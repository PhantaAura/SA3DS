#ifndef SA3DS_CHECKPOINT_TASK_H
#define SA3DS_CHECKPOINT_TASK_H
#include <stdint.h>
/*44F540 parent task, with44F430 contact and44F150 angular update. Caller owns
 * SET lifetime/clip, dynamic-land children, prior CCL and player restart data.*/
typedef struct {
    uint8_t action;
    int32_t post_x,post_y,velocity_x,velocity_y,time_display;
    int16_t effect_count;
} SA3DSCheckpointTask;
typedef struct {
    uint8_t clipped,persisted,player_present,prior_collision,other_is_player0;
    int16_t activation_spin; /*44F3B0 signed speed ->44F4EB truncated BAMS*/
} SA3DSCheckpointInput;
typedef struct {
    uint8_t initialize,display,active_material,sync_children,enqueue_collision;
    uint8_t capture_restart,persist_set,play_sound,emit_effect,display_time;
    int32_t draw_post_x,draw_post_y,child_post_x,child_post_y;
    uint16_t sound_id,effect_identifier;
} SA3DSCheckpointResult;
int sa3ds_checkpoint_task_tick(SA3DSCheckpointTask *task,
    const SA3DSCheckpointInput *input,SA3DSCheckpointResult *out);
/*44F4D6..EB; no fabricated minimum impulse. Source speed/yaw calculation is a
 * caller service, distinct from this conversion of the resulting signed speed.*/
int sa3ds_checkpoint_spin(double signed_speed,int16_t *spin);
#endif
