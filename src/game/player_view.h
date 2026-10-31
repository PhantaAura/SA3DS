#ifndef SA3DS_PLAYER_VIEW_H
#define SA3DS_PLAYER_VIEW_H
#include "../sa1/physics/input.h"
#include "../sa1/physics/player_state.h"

/* Native RH chase view looks along (sin(yaw),0,cos(yaw)). Its screen-right
 * axis is (-cos(yaw),0,sin(yaw)), opposite the core's positive input angle.
 * Apply exactly once at the screen-input/core boundary; retain raw provenance. */
SA1Input sa3ds_view_input(SA1Input screen_input);
/* Unpitched RH view used by White Zone geometry; point is player-relative. */
SA1Vec3 sa3ds_view_point(SA1Vec3 point, float camera_yaw);
/* Original Sonic ACTION model -X is forward. Citro3D RotateY maps +X to
 * (cos(angle),0,-sin(angle)); this returns the corresponding RH view angle. */
float sa3ds_view_model_yaw(float player_yaw, float camera_yaw);
#endif
