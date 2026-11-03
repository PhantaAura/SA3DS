#ifndef SA3DS_SARU_ACTOR_H
#define SA3DS_SARU_ACTOR_H
#include "saru.h"
#include "enemy_damage.h"
typedef struct {
    SA1Vec3 origin,position,saved_position,scale;
    int32_t angles[3],target_orbit,player_yaw;
    uint32_t idle_phase,hop_phase,age;
    uint16_t status,counter,packed_tag;
    uint8_t action,previous_action,mode,wait,death_byte9,death_byte10;
    float squash_velocity,bob,bomb_scale,damage_velocity_y;
    SA3DSSaruAnimation animation;
} SA3DSSaruActor;
typedef struct {
    SA1Vec3 position,scale;
    int32_t angles[3];
    float bob;uint8_t variant;
    SA3DSSaruAnimation animation;
} SA3DSSaruDraw;
typedef struct {
    uint8_t draw,display,enqueue_collision,destroy,death,projectile;
    uint8_t dust_bursts,sound_count;
    uint16_t sound_ids[2];
    uint8_t sound_positional[2]; /*0=424880 actor/120,1=424FC0 position*/
    SA3DSSaruDraw logic_draw;
    /* Projectile request is generated in the original pre-frame-increment
     * phase, with source joint14/ACTION/frame and task rotation still live.
     * Actual projectile construction, collision and effects are caller-owned. */
    SA3DSSaruDraw projectile_pose;
} SA3DSSaruActorResult;
typedef struct {
    void *context;
    double (*sine)(void *context,uint16_t angle);
    /* Signed trunc(atan2(y,x)*65536*.15915497620314795), not exact1/tau. */
    int32_t (*atan_bams)(void *context,double y,double x);
    uint16_t (*random15)(void *context);
    /* Source49EE30(task,1) query. Required for task status2000. */
    double (*floor_height)(void *context,const SA3DSSaruActor *actor);
    /* Source4CE030; invoked at source call sites, may mutate squash/flags.
     * A shared collision/damage implementation must provide this callback. */
    int (*damage)(void *context,SA3DSSaruActor *actor);
} SA3DSSaruServices;
typedef struct {
    SA1Vec3 player_position;
    const uint16_t *action_frames; /*six counts read from owner's SA3C*/
    int visible,render_suppressed,collision_flag0;
} SA3DSSaruActorInput;
/* Connect original shared4CE030 decisions to this actor's real enemy fields.
 * The world input supplies prior CCL/cache/player/explosion state; live actor
 * status and position override its corresponding input fields. Outputs must
 * be executed at each source call, not accumulated once per actor lifetime. */
int sa3ds_saru_damage_check(SA3DSSaruActor *actor,const SA3DSEnemyDamageInput *world,
    SA3DSCCLQueryCursor *cursor,SA3DSEnemyDamageResult *out);
int sa3ds_saru_actor_init(SA3DSSaruActor *actor,const SA3DSMovementPlacement *placement,
    const SA3DSSaruServices *services);
/* Ordinary4ACF80 action dispatch/common tail, after4258F0/46C010 decisions.
 * Caller owns SET activation/unload, special-mode4AB7C0, source CCL pass,
 * projectile/death/effect execution, read-only display and native rendering.
 * No approximate collision/damage service or substitute projectile is supplied.
 * Outputs describe original requests, not proof that those systems ran. */
int sa3ds_saru_actor_tick(SA3DSSaruActor *actor,const SA3DSSaruActorInput *input,
    const SA3DSSaruServices *services,SA3DSSaruActorResult *out);
#endif
