#ifndef SA3DS_SARU_H
#define SA3DS_SARU_H
#include "movement_objects.h"
#define SA3DS_SARU_PACKAGE_MAX_BYTES (256u*1024u)
typedef struct {
    SA3DSMovementObjects placements;
    const uint8_t *collider; /*48 source CCL bytes, borrowed owner payload */
    float load_squared,unload_squared,visibility_radius,display_scale;
} SA3DSSaruDefinitions;
typedef struct {
    SA1Vec3 spawn_origin,task_position,task_scale;
    int32_t task_angles[3],initial_angle_y;
    uint16_t packed_tag_low,task_status,enemy_flags;
    uint8_t mode,index_seed;
    float animation_frame;
    uint32_t behavior_flags;
    float parameter_cc,parameter_d0,parameter_3c;
} SA3DSSaruInitial;
int sa3ds_saru_definitions_open(SA3DSSaruDefinitions *out,const void *data,size_t size);
/*4AD140/4AB240 fresh zeroed-allocation profile. sine/cosine of SET angleX
 * come from the owner's source trig. Do not treat X as physical pitch or Z as
 * an ordinary rotation: Z's two bytes are consumed separately by the source.
 * Shared enemy allocation/AI/update, animation clips and rendering are not
 * performed by this initializer. Failure clears out. */
int sa3ds_saru_initialize(const SA3DSMovementPlacement *placement,uint16_t random15,
    double sine_x,double cosine_x,SA3DSSaruInitial *out);
/* Original ACTION identities in owner saru.sa3c, not inferred behavior names. */
enum { SA3DS_SARU_ACTION_COUNT=6 };
typedef struct {
    uint16_t current_action,pending_action,enemy_flags;
    float current_frame,pending_frame,blend_weight,blend_rate;
} SA3DSSaruAnimation;
void sa3ds_saru_animation_init(SA3DSSaruAnimation *state);
/* Source state writers set flag200, pending ACTION/frame/rate. They do not
 * clear an existing blend weight when a request interrupts another blend. */
int sa3ds_saru_animation_request(SA3DSSaruAnimation *state,uint16_t action,
    float frame,float blend_rate);
/* Call exactly when source4AB3F0/4AB5E0 reaches its draw branch. Capture the
 * pre-advance draw pose, then advance blend/completion.4AB2B0's later display
 * callback must only read state. eligible includes the relevant helper's
 * visibility/global suppression gates; do not advance per rendered frame. */
int sa3ds_saru_animation_logic_draw(SA3DSSaruAnimation *state,int eligible,
    SA3DSSaruAnimation *draw_pose);
#endif
