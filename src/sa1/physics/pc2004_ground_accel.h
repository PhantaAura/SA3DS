#ifndef SA1_PC2004_GROUND_ACCEL_H
#define SA1_PC2004_GROUND_ACCEL_H
#include <stdint.h>
#include "pc2004_motion.h"

typedef struct {
    /* Already task-local Gravity*weight + external force, including source
     * unstick/scaling and resistance. Capture BEFORE turning helpers. */
    SA1PC2004LocalVector force_after_resistance; /* A,N,Z */
    float input_force; /* selected local F, not necessarily motionwk2+34 */
    SA1PC2004LocalVector speed_after_turn;
    float up, friction_multiplier;
    int analog_active, surface_flag4;
    /* Source local14: normally0; analog task status2000 branch sets-.8.
     * Added to output normal only, after forward/side friction clamps. */
    float normal_addend;
} SA1PC2004GroundAccelInput;
typedef struct {
    SA1PC2004LocalVector add_velocity;
    float animation_residual;
    int wrote_residual; /* distinguish source store from tail-initialized0 */
} SA1PC2004GroundAccelResult;

/* Ordinary44C923..44CDE4 clamp, excluding delegated status/flag paths.
 * Explicit inputs preserve force-before-turn / clamp-after-turn ordering.
 * No speed integration, static contact cancellation or input selection here.
 * Retained double intermediates and original f32 stores are used; this is not
 * an x87 extended-precision emulator. Finite inputs only; failure clears out. */
int sa1_pc2004_ground_accel_clamp(const SA1PC2004GroundAccelInput *input,
    const SA1SteamPlayerParameters *parameters,SA1PC2004GroundAccelResult *out);
/* Pure local-speed part443F50..44412B. The caller supplies the analog result
 * queried during integration, before the signed control timer tail. Source
 * auxiliary flags are player+1E4; these are not surface/player+5C flags.
 * World transform, velocity clamps, pending displacement and contact follow. */
int sa1_pc2004_pgetspeed_local(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector add_velocity,int analog_active,float up,
    uint32_t auxiliary_flags,const SA1SteamPlayerParameters *parameters,
    SA1PC2004LocalVector *out);
/*4432E0, zero pending collision impulse; force is already task-local.
 * Used by source spring action10 while normal speed>=nocon_speed. */
int sa1_pc2004_noinput_add_velocity(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector force,const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *parameters,SA1PC2004LocalVector *out);
#endif
