#ifndef SA1_PHYSICS_STEAM_AIR_H
#define SA1_PHYSICS_STEAM_AIR_H

#include "movement.h"
#include "steam_input.h"
#include "steam_player_parameters.h"

/* Evidence-driven Steam SADX air/jump diagnostics. These values are for
 * parity work and are not part of the shipping player state. */
typedef struct {
    float forward_speed_before;
    float vertical_speed_before;
    float side_speed_before;
    float forward_speed_after;
    float vertical_speed_after;
    float side_speed_after;
    float accel_forward;
    float accel_vertical;
    float accel_side;
    float requested_force;
    float filtered_input_magnitude;
    unsigned angle_delta_bams;
    unsigned turn_step_bams;
    int had_analog;
    int used_air_brake;
    int blocked_wide_angle_accel;
    int doubled_air_accel;
} SA1SteamAirDebug;

/* Exact Sonic table values are used by these helpers. The initial jump write
 * is directly evidenced in the supported Steam EXE: pwp->Speed.y =
 * pwp->p.jmp_y_spd. */
void sa1_steam_sonic_begin_jump(SA1PlayerState *player);

/* Steam helper 0x004DBA90 increments the jump timer while the jump input is
 * held and the previous timer is below jump2_timer. Sonic's airborne state
 * then adds jmp_addit * 0.8 to vertical speed. Returns 1 when applied. */
int sa1_steam_sonic_apply_jump_hold(SA1PlayerState *player, int jump_held);

/* Flat-world specialization of Steam PGetAccelerationAir (0x004DDEC0) plus
 * the relevant PGetSpeed integration. Surface-relative gravity/collision is
 * deliberately deferred to 0.0.8.7. */
void sa1_steam_sonic_air_step(SA1PlayerState *player,
                              const SA1Input *input,
                              float camera_yaw,
                              SA1SteamAirDebug *debug_out);

#endif
