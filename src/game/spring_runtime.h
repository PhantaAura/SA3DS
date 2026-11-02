#ifndef SA3DS_SPRING_RUNTIME_H
#define SA3DS_SPRING_RUNTIME_H
#include "spring_launch.h"
#include "../sa1/physics/input.h"
#include "../sa1/physics/steam_player_parameters.h"
#include "../sa1/physics/pc2004_motion.h"
#include "../sa1/physics/surface_frame.h"
/* Source object task state. Placement lifetime/loading and the global CCL
 * queue remain caller-owned; one tick here is one actual spring object tick. */
typedef struct { uint8_t action, cooldown, player_index; } SA3DSSpringActor;
typedef struct {
    int collision_player; /* already-arbitrated previous CCL result: -1,0,1 */
    int paused, skip_update;
    int player_grounded;
    float player_local_forward;
    SA1Vec3 player_task_origin; /* source foot/task position, not port support center */
} SA3DSSpringTickInput;
typedef struct {
    uint8_t enqueue_collision, draw, launched, player_index;
    SA3DSSpringLaunch launch;
} SA3DSSpringTickResult;
void sa3ds_spring_actor_reset(SA3DSSpringActor *actor);
/* Transactional failure (including unsupported off-ground getter scratch)
 * leaves actor unchanged and clears result. No guessed collision arbitration. */
int sa3ds_spring_actor_tick(SA3DSSpringActor *actor,
    const SA3DSMovementPlacement *placement,const SA3DSSpringTrig *trig,
    const SA3DSSpringTickInput *input,SA3DSSpringTickResult *out);

/* SpringB writes all task axes, not just launch direction. Preserves raw
 * supplied trig matrix boundaries: port forward=source localX, up=localY,
 * right=-localZ. Class1 retains its existing player frame and rejects here. */
int sa3ds_springb_physical_frame(const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,SA1SurfaceFrame *out);

/* Ordinary Sonic command3/4 consumer 495FA0. Wrappers have already prepared
 * velocity/angles and set task status0x1000; damage/death/forced-context gates
 * must be handled by the caller first. Ground status low bits are preserved. */
typedef struct {
    uint16_t task_status, animation_request;
    int16_t control_timer;
    uint8_t sonic_action;
} SA3DSSpringCommandResult;
int sa3ds_spring_command_consume(const SA3DSSpringCommand *command,
    uint16_t source_task_status,int16_t prior_timer,SA3DSSpringCommandResult *out);

typedef enum {
    SA3DS_SPRING_MOVE_AIR_CONTROL, /* 44B9C0, no jump-hold helper */
    SA3DS_SPRING_MOVE_UNCONTROLLED, /* 4432E0, before PGetSpeed */
    SA3DS_SPRING_MOVE_FALL /* newly selected action12; ordinary fall dispatch */
} SA3DSSpringMovement;
typedef struct {
    uint8_t sonic_action;
    uint16_t animation_request;
    SA3DSSpringMovement movement;
} SA3DSSpringActionResult;
/* Call in Sonic_Act1 selection phase BEFORE movement. World Y decides the
 * apex transition; local Speed.y independently decides action10 acceleration. */
int sa3ds_spring_action_select(uint8_t sonic_action,uint16_t animation_request,
    float world_velocity_y,float local_velocity_y,
    const SA1SteamPlayerParameters *parameters,SA3DSSpringActionResult *out);
/* 4432E0 acceleration with zero pending player impulse. Gravity/external force
 * must already be source-frame transformed. Does not integrate Speed/Position.
 * Handles the inherited grounded launch tick; unsupported status4000 rejects. */
int sa3ds_spring_uncontrolled_add_velocity(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector source_local_force,const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *parameters,SA1PC2004LocalVector *out);
int sa3ds_spring_control_blocked(int16_t timer);
int16_t sa3ds_spring_control_timer_tail(int16_t timer);
SA1Input sa3ds_spring_filter_analog(SA1Input input,int16_t timer);
/* Exact ordinary SpringB form0 vs Sonic primary form7 narrow-phase route:
 * 41FEC0->41FD00->41AA00. Caller supplies the source primary collider center
 * (4915F0 task+rotated center_height) and radius, not Sonic parameter rad.
 * Queue/category filtering and first-hit ordering remain caller-owned. */
int sa3ds_springb_primary_overlap(const SA3DSMovementPlacement *placement,
    SA1Vec3 player_primary_center,float player_primary_radius);
typedef struct {
    SA1Vec3 task_origin, center;
    float radius, half_height, foot_radius;
    unsigned gravity_axis; /* source global3 is ordinary world-Y gravity */
} SA3DSSpringPrimaryCollider;
/* Returns1 hit,0 miss,-1 unsupported/invalid. Class1 follows41CCD0 with source
 * form7 dimensions and world-Y gravity only; class2 follows sphere pair above.
 * Default unchanged Sonic record is radius4/halfheight4/footradius.65. */
int sa3ds_spring_primary_overlap(const SA3DSMovementPlacement *placement,
    const SA3DSSpringPrimaryCollider *primary);
/* Translated CCL hit-list query41C6E0/41CBC0, separate from geometry. Source
 * has16 slots and stops on other_shape==-1. start_slot is the explicit output
 * of the caller-owned shared query cursor41B9F0 (normally0 for a new task).
 * Category0 can name an unsupported player: do not search past that hit. */
typedef struct {
    int8_t other_shape;
    uint8_t task_present, ccl_present, player_index;
    int16_t ccl_category;
} SA3DSSpringHitEntry;
typedef struct { unsigned slot; uint8_t player_index; } SA3DSSpringHitResult;
int sa3ds_spring_query_player_hit(const SA3DSSpringHitEntry *entries,
    unsigned entry_count,unsigned start_slot,SA3DSSpringHitResult *out);
#endif
