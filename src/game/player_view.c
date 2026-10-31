#include "player_view.h"
#include <math.h>

SA1Input sa3ds_view_input(SA1Input input) {
    input.move_x = -input.move_x;
    return input;
}

SA1Vec3 sa3ds_view_point(SA1Vec3 p, float yaw) {
    float c=cosf(yaw), s=sinf(yaw);
    return (SA1Vec3){-p.x*c+p.z*s, p.y, -p.x*s-p.z*c};
}

float sa3ds_view_model_yaw(float player_yaw, float camera_yaw) {
    return player_yaw-camera_yaw-1.57079632679489661923f;
}
