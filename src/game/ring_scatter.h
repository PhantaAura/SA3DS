#ifndef SA3DS_RING_SCATTER_H
#define SA3DS_RING_SCATTER_H
#include "../sa1/physics/player_state.h"
/*Source44FD10 task state, separate from SET/direct/group ring actors. Native
 * task allocation, source CCL, art/audio and world query adapters are callers.*/
typedef struct {
    SA1Vec3 position;
    int32_t angles[3];
    float spin,decay_age,scale_or_floor,radial_x,launch_y,radial_z;
    float cached_floor,floor_phase;
    uint16_t age;
    uint8_t action,collection_delay,collision_id;
} SA3DSRingScatter;
typedef struct {
    void *context;
    uint16_t (*random15)(void *context);
    double (*sine)(void *context,uint16_t angle);
    int32_t (*atan_bams)(void *context,double y,double x);
    /*49E920 query at position+worldY6.88, writes source task angles.
     * Return source sentinel -1000000 for absent support.*/
    double (*floor)(void *context,SA1Vec3 position,int32_t angles[3]);
    /*Source Ninja Rz(taskZ),Rx(taskX) matrix; no taskY rotation.*/
    SA1Vec3 (*tilt)(void *context,const int32_t angles[3],SA1Vec3 vector);
} SA3DSRingScatterServices;
typedef struct {
    uint32_t global_frame;
    uint8_t hit,hit_is_primary_player,primary_dead,paused,render_suppressed;
} SA3DSRingScatterInput;
typedef struct {
    uint8_t initialize_collision,clear_hit,enqueue_collision,award_ring,sound7;
    uint8_t reset_pickup_effect,normal_draw,pickup_effect,destroy;
    uint8_t display_callback; /*0 unchanged,1=44FC90,2=44FCE0,3=44FC80*/
    SA1Vec3 collision_position;
    int particle_frames[3];
} SA3DSRingScatterResult;
/*Global spawn_phase is03B42FE8 shared across spawned tasks, cycling0..8.
 * Creation/list order and source RNG order must be preserved by the caller.
 * Returns0 for missing services/bad state,1 for an original task update.
 * No fabricated GPU, collision, sound or pickup executors are supplied.*/
int sa3ds_ring_scatter_tick(SA3DSRingScatter *actor,int32_t *spawn_phase,
    const SA3DSRingScatterInput *input,const SA3DSRingScatterServices *services,
    SA3DSRingScatterResult *out);
#endif
