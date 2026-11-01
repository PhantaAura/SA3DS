#ifndef SA3DS_SPRING_LAUNCH_H
#define SA3DS_SPRING_LAUNCH_H
#include "movement_objects.h"
/* Caller supplies sine/cosine of each original SET angle (X,Y,Z), using its
 * chosen source-neutral math provider or owner-generated source LUT. */
typedef struct { float sine[3], cosine[3]; } SA3DSSpringTrig;
typedef struct {
    SA3DSSpringCommand command;
    SA1Vec3 endpoint, position, world_velocity, local_velocity;
    int32_t task_angles[3];
    uint8_t angles_written, local_velocity_written;
} SA3DSSpringLaunch;
/* Pure preparation of 7A46C0 after a recorded spring hit (+8 == 0.1).
 * Does not apply physics, arbitrate hits, consume commands, or advance tasks.
 * Grounded means source status&3 !=0: 441990 otherwise leaves its output
 * scratch unwritten; that unsupported branch is rejected. Class1 preserves
 * player angles and needs a separate world->local conversion at application.
 * Failure clears out. Supplied trig must correspond to placement angles. */
int sa3ds_spring_launch_prepare(const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig, SA1Vec3 player_position, int grounded,
    float local_forward, SA3DSSpringLaunch *out);
#endif
