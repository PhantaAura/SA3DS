#ifndef SA3DS_GAME_STAGE_H
#define SA3DS_GAME_STAGE_H

#include "../sa1/physics/ground.h"

/* Original test geometry retained as the permanent regression course. */
SA1Surface sa3ds_movement_lab_surface(void *context, float x, float z);
SA1Contact sa3ds_movement_lab_contact(void *context, SA1Vec3 position,
                                      SA1Vec3 probe_direction,
                                      float max_distance);
const SA1PhysicsWorld *sa3ds_movement_lab_world(void);

#endif
