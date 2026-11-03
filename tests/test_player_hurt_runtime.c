#include "player_damage_player.h"
#include "sonic_animation_runtime.h"
#include "../src/sa1/physics/steam_flat.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static SA1Surface floor_surface(void*c,float x,float z){(void)c;(void)x;(void)z;return (SA1Surface){.ground_y=0,.normal={0,1,0},.valid=true};}
static uint16_t random15(void*c){(void)c;return 0;}
int main(void){
    SA1PhysicsWorld world={.query_surface=floor_surface,.reset_position={0,1,0},.reset_min_x=-10000,.reset_max_x=10000,.reset_min_y=-10000,.reset_min_z=-10000,.reset_max_z=10000};
    SA1PlayerState p;sa1_player_reset(&p,&world);p.vel=(SA1Vec3){.25f,0,1};
    const SA1SteamPlayerParameters*parameters=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA3DSPlayerDamageWorld damage_world={.stage_id=1,.death_command_allowed=1,.random15=random15};
    SA3DSSonicAnimationRuntime animation={0};SA3DSSonicAnimationSync sync={0};
    SA1Input input={0};int16_t rings=7;unsigned hurt_frames=0,air=0,ground=0,clamps=0,landing_pose=0;
    for(unsigned tick=0;tick<100;tick++){
        uint16_t previous=animation.initialized?animation.requested_runtime_id:0;
        SA3DSPlayerDamageResult result;
        assert(sa3ds_player_damage_prepare(&p,&input,tick==0?4:0,previous,&rings,&damage_world,parameters,&result)==1);
        if(!tick){assert(result.accepted&&p.pc2004_action_state==16&&p.action_state==SA1_ACTION_HURT&&rings==0&&p.pc2004_hurt_timer==120);
            assert(fabsf(p.vel.z+parameters->run_speed)<1e-6f&&p.vel.y==parameters->jmp_y_spd&&p.vel.x==.25f);}
        sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        if(p.pc2004_action_state==16){hurt_frames++;if(p.grounded)ground++;else air++;}
        SA3DSSonicAnimationObservation observation=sa3ds_sonic_animation_observe(&sync,&p,parameters);
        SA1PlayerState before=p;
        SA3DSSonicAnimationSelection selected=sa3ds_sonic_animation_select(&animation,&p,&observation);
        assert(!memcmp(&p,&before,sizeof(p)));
        if(selected.requested_runtime_id==23){assert(selected.sample_frame<=21);if(selected.sample_frame==21)clamps++;}
        if(selected.requested_runtime_id==24)landing_pose++;
    }
    assert(hurt_frames&&air&&ground&&clamps&&landing_pose);
    assert(p.pc2004_hurt_timer==21&&p.pc2004_action_state==1&&p.action_state==SA1_ACTION_GROUNDED);
    printf("Public hurt/animation: PASS %u hurt frames/%u air/%u ground, clamp23=%u/landing24=%u, source animation completion resumes movement with immunity still active\n",hurt_frames,air,ground,clamps,landing_pose);
}
