#include "movement.h"
#include <math.h>
#include <string.h>

#define SA1_PI 3.14159265358979323846f
#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

static float wrap_angle(float angle) {
    while (angle > SA1_PI) angle -= 2.0f * SA1_PI;
    while (angle < -SA1_PI) angle += 2.0f * SA1_PI;
    return angle;
}

static float approach_angle(float angle, float target, float step) {
    float difference = wrap_angle(target - angle);
    if (difference > step) difference = step;
    if (difference < -step) difference = -step;
    return wrap_angle(angle + difference);
}

float sa1_horizontal_speed(const SA1PlayerState *player) {
    return sqrtf(player->vel.x*player->vel.x + player->vel.z*player->vel.z);
}

void sa1_player_reset(SA1PlayerState *player, const SA1PhysicsWorld *world) {
    memset(player, 0, sizeof(*player));
    player->pos = world->reset_position;
    player->yaw = 0.0f;
    player->grounded = true;
    player->peak_y = player->pos.y;
    player->ground_normal = (SA1Vec3){0.0f, 1.0f, 0.0f};
    player->surface_forward = (SA1Vec3){0.0f, 0.0f, 1.0f};
    player->surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
    player->support_distance = 1.0f;
    player->surface_flags = 0u;
    player->surface_id = -1;
    player->action_state = SA1_ACTION_GROUNDED;
}

static void update_surface_state(SA1PlayerState *player, SA1Surface surface) {
    if (!surface.valid) {
        player->surface_id = -1;
        player->ground_normal = (SA1Vec3){0.0f, 1.0f, 0.0f};
        player->ground_slope_radians = 0.0f;
        return;
    }
    player->surface_id = surface.surface_id;
    player->ground_normal = surface.normal;
    player->ground_slope_radians = surface.slope_radians;
}

void sa1_physics_step(SA1PlayerState *player, const SA1Input *input,
                      float camera_yaw, const SA1PhysicsWorld *world,
                      const SA1PhysicsConstants *constants) {
    float mx = input->move_x;
    float mz = input->move_z;
    float magnitude = sqrtf(mx*mx + mz*mz);
    bool has_input = magnitude > 0.001f;
    float speed = sa1_horizontal_speed(player);

    if (has_input) {
        if (magnitude > 1.0f) { mx /= magnitude; mz /= magnitude; magnitude = 1.0f; }
        /* Provisional smooth response retained from Movement Lab 0.0.7. */
        float response = magnitude * magnitude * (3.0f - 2.0f * magnitude);
        float desired = camera_yaw + atan2f(mx, mz);
        float speed_t = CLAMP(speed / constants->top_speed, 0.0f, 1.0f);
        float turn = constants->turn_low +
                     (constants->turn_high - constants->turn_low) * speed_t;
        player->yaw = approach_angle(player->yaw, desired,
                                     turn * (0.35f + 0.65f * response));

        float forward_x = sinf(player->yaw), forward_z = cosf(player->yaw);
        float desired_x = sinf(desired), desired_z = cosf(desired);
        float dot = speed > 0.01f ?
            (player->vel.x * desired_x + player->vel.z * desired_z) / speed : 1.0f;
        float accel = (player->grounded ? constants->accel_ground : constants->accel_air) * response;
        float drag = player->grounded ? constants->drag_ground : constants->drag_air;
        if (player->grounded && dot < -0.30f) {
            accel = constants->brake_accel * response;
            drag = constants->drag_brake;
        }
        player->vel.x = player->vel.x * drag + forward_x * accel;
        player->vel.z = player->vel.z * drag + forward_z * accel;
    } else {
        float drag = player->grounded ? constants->drag_ground : constants->drag_air;
        player->vel.x *= drag;
        player->vel.z *= drag;
    }

    SA1Surface surface = world->query_surface(world->context, player->pos.x, player->pos.z);
    if (player->grounded && surface.valid) {
        player->vel.z += -surface.grad_z * constants->slope_push;
        player->vel.x += -surface.grad_x * constants->bank_push;
    }

    speed = sa1_horizontal_speed(player);
    if (speed > constants->top_speed) {
        float scale = constants->top_speed / speed;
        player->vel.x *= scale;
        player->vel.z *= scale;
    }

    if (input->jump_pressed && player->grounded) {
        player->vel.y = constants->jump_speed;
        player->grounded = false;
        player->jump_ticks = 0;
    }
    if (!input->jump_held && !player->grounded && player->vel.y > 0.0f)
        player->vel.y *= constants->jump_cut;

    if (!player->grounded) {
        player->vel.y -= constants->gravity;
        if (player->vel.y < constants->max_fall) player->vel.y = constants->max_fall;
        player->jump_ticks++;
    }

    player->pos.x += player->vel.x;
    player->pos.y += player->vel.y;
    player->pos.z += player->vel.z;

    surface = world->query_surface(world->context, player->pos.x, player->pos.z);
    if (surface.valid && player->vel.y <= 0.0f &&
        player->pos.y >= surface.ground_y - 0.35f &&
        player->pos.y <= surface.ground_y + 1.6f) {
        player->pos.y = surface.ground_y + 1.0f;
        player->vel.y = 0.0f;
        player->grounded = true;
        player->jump_ticks = 0;
    } else if (!surface.valid || player->pos.y > surface.ground_y + 1.6f) {
        player->grounded = false;
    }

    update_surface_state(player, surface);
    player->action_state = player->grounded ? SA1_ACTION_GROUNDED : SA1_ACTION_AIRBORNE;

    if (player->pos.y < world->reset_min_y ||
        player->pos.x < world->reset_min_x || player->pos.x > world->reset_max_x ||
        player->pos.z < world->reset_min_z || player->pos.z > world->reset_max_z)
        sa1_player_reset(player, world);

    speed = sa1_horizontal_speed(player);
    if (speed > player->peak_speed) player->peak_speed = speed;
    if (player->pos.y > player->peak_y) player->peak_y = player->pos.y;
    player->sim_frame++;
}
