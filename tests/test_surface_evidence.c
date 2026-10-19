#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "steam_player_parameters.h"
#include "steam_surface_evidence.h"

static int nearf(float a, float b, float eps) { return fabsf(a - b) <= eps; }

int main(void) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    float f = 0.0f, s = 0.0f;

    sa1_sadx_surface_force_scales(0.95f, 0.2f, p, &f, &s);
    assert(nearf(f, 1.0f, 1e-6f) && nearf(s, 1.0f, 1e-6f));

    sa1_sadx_surface_force_scales(0.80f, 0.2f, p, &f, &s);
    assert(nearf(f, 1.0f, 1e-6f) && nearf(s, 1.4f, 1e-6f));

    sa1_sadx_surface_force_scales(0.60f, 0.2f, p, &f, &s);
    assert(nearf(f, 1.0f, 1e-6f) && nearf(s, 2.0f, 1e-6f));

    sa1_sadx_surface_force_scales(0.40f, 0.2f, p, &f, &s);
    assert(nearf(f, 4.225f, 1e-6f) && nearf(s, 4.225f, 1e-6f));

    /* Speed bypasses the steep low-speed force scaling branch. */
    sa1_sadx_surface_force_scales(0.40f, p->run_speed, p, &f, &s);
    assert(nearf(f, 1.0f, 1e-6f) && nearf(s, 1.0f, 1e-6f));

    assert(sa1_sadx_surface_primary_unstick_branch(0.0f, 0.7f, 2.0f));
    assert(!sa1_sadx_surface_primary_unstick_branch(0.1f, 0.7f, 2.0f));
    assert(!sa1_sadx_surface_primary_unstick_branch(0.0f, 0.6f, 2.0f));
    assert(!sa1_sadx_surface_primary_unstick_branch(0.0f, 0.7f,
            SA1_SADX_UNSTICK_SPEED_PC2004_EVIDENCE));

    assert(nearf(sa1_sadx_surface_unstick_normal_bias(-0.45f, 2.0f, p->weight),
                 -p->weight * 5.0f, 1e-6f));
    assert(nearf(sa1_sadx_surface_unstick_normal_bias(-0.35f, 2.0f, p->weight),
                 -p->weight * 0.8f, 1e-6f));
    assert(nearf(sa1_sadx_surface_unstick_normal_bias(-0.20f, 2.0f, p->weight),
                 -p->weight * 0.4f, 1e-6f));
    assert(nearf(sa1_sadx_surface_unstick_normal_bias(-0.20f, 1.0f, p->weight),
                 0.0f, 1e-6f));

    /* 0.0.8.8's standalone 0.3/1.16 forced-support-loss helper was removed.
       1.16 remains verified here only as a PGetAcceleration unstick threshold. */

    printf("surface evidence branch tests: PASS\n");
    return 0;
}
