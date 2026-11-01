#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "steam_flat.h"
#include "movement.h"
#include "surface_frame.h"
#include "pc2004_motion.h"
static SA1Vec3 support_normal;
static SA1Surface plane(void *ctx,float x,float z) {
    (void)ctx;
    return (SA1Surface){.ground_y=-(support_normal.x*x+support_normal.z*z)/support_normal.y,
        .grad_x=-support_normal.x/support_normal.y,.grad_z=-support_normal.z/support_normal.y,
        .normal=support_normal,.valid=true};
}
int main(void) {
    SA1PhysicsWorld world={.query_surface=plane,.reset_position={0,1,0},
        .reset_min_x=-1000,.reset_max_x=1000,.reset_min_y=-1000,
        .reset_min_z=-1000,.reset_max_z=1000};
    const float pi=3.14159265358979323846f;
    assert(sa1_pc2004_turn_delta(pi)==pi);
    assert(sa1_pc2004_turn_delta(-pi)==pi);
    assert(sa1_pc2004_turn_delta(pi+0.000002f)==pi);
    assert(sa1_pc2004_turn_delta(-pi-0.000002f)==pi);
    assert(sa1_pc2004_turn_delta(pi+0.001f)<0);
    assert(sa1_pc2004_turn_delta(-pi-0.001f)>0);
    float nx=.0717043653f;
    unsigned tested=0;
    for(unsigned variant=0;variant<256;variant++) {
        support_normal=sa1_vec3_normalize((SA1Vec3){nx,.996830881f,-.0344491675f});
        SA1PlayerState p;sa1_player_reset(&p,&world);
        SA1Input input={.move_z=-1};
        float camera=(variant&1)?0.000002f:-0.000002f;
        sa1_physics_step_steam_sonic_flat(&p,&input,camera,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        /* 0x438368 sign-extends task-angle 0x8000, chooses decreasing
         * task Y. The port's yaw has opposite sign, so its turn is positive.
         * Tiny float differences in a support plane must not reverse it. */
        if(!(p.yaw>0)){
            fprintf(stderr,"half-turn sign mismatch normalX=%.9g yaw=%.9g variant=%u\n",nx,p.yaw,variant);
            return 1;
        }
        nx=nextafterf(nx,1);tested++;
    }
    printf("PC2004 half-turn: PASS (%u neighboring source-plane normals; deterministic task-angle tie)\n",tested);
    return 0;
}
