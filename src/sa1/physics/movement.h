#ifndef SA1_PHYSICS_MOVEMENT_H
#define SA1_PHYSICS_MOVEMENT_H

#include "ground.h"
#include "input.h"
#include "physics_constants.h"
#include "player_state.h"

#define SA1_SIMULATION_HZ 60u

void sa1_player_reset(SA1PlayerState *player, const SA1PhysicsWorld *world);
void sa1_physics_step(SA1PlayerState *player, const SA1Input *input,
                      float camera_yaw, const SA1PhysicsWorld *world,
                      const SA1PhysicsConstants *constants);
float sa1_horizontal_speed(const SA1PlayerState *player);

#endif
