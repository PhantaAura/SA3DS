#include "steam_surface_evidence.h"

#include <math.h>

void sa1_sadx_surface_force_scales(float up_y,
                                   float forward_speed,
                                   const SA1SteamPlayerParameters *p,
                                   float *out_forward_scale,
                                   float *out_side_scale) {
    float forward_scale = 1.0f;
    float side_scale = 1.0f;
    float speed = fabsf(forward_speed);

    if (up_y >= 0.5f || speed >= p->run_speed) {
        if (up_y >= 0.7f || speed >= p->run_speed) {
            if (!(up_y >= 0.87f || speed >= p->jog_speed))
                side_scale = 1.4f;
        } else {
            side_scale = 2.0f;
        }
    } else {
        forward_scale = 4.225f;
        side_scale = 4.225f;
    }

    if (out_forward_scale) *out_forward_scale = forward_scale;
    if (out_side_scale) *out_side_scale = side_scale;
}

int sa1_sadx_surface_primary_unstick_branch(float up_y,
                                            float cross_y_abs,
                                            float forward_speed) {
    return up_y < SA1_SADX_UNSTICK_UP_PRIMARY_PC2004_EVIDENCE &&
           fabsf(cross_y_abs) > SA1_SADX_UNSTICK_CROSS_PC2004_EVIDENCE &&
           forward_speed > SA1_SADX_UNSTICK_SPEED_PC2004_EVIDENCE;
}

float sa1_sadx_surface_unstick_normal_bias(float up_y,
                                           float forward_speed,
                                           float weight) {
    if (forward_speed <= SA1_SADX_UNSTICK_SPEED_PC2004_EVIDENCE)
        return 0.0f;
    if (up_y < SA1_SADX_UNSTICK_UP_5X_PC2004_EVIDENCE)
        return -weight * 5.0f;
    if (up_y < SA1_SADX_UNSTICK_UP_08X_PC2004_EVIDENCE)
        return -weight * 0.8f;
    if (up_y < SA1_SADX_UNSTICK_UP_04X_PC2004_EVIDENCE)
        return -weight * 0.4f;
    return 0.0f;
}
