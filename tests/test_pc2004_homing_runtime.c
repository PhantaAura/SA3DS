/*Controller-level homing handoff, not a retail movement trace.*/
#include "pc2004_homing.h"
#include "pc2004_task_frame.h"
#include "steam_flat.h"
#include "steam_player_parameters.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define TAU 6.283185307179586476925286766559
static double sine(void*c,uint16_t a){(void)c;return sin(a*(TAU/65536.));}
static uint16_t bearing(void*c,float z,float x){(void)c;return (uint16_t)(int32_t)(atan2((double)z,x)*65536.*.15915497620314795);}
static int near(float a,float b){return fabsf(a-b)<.00002f;}
static SA1PC2004HomingPose target;
static int resolve(void*c,uint32_t id,SA1PC2004HomingPose*p){(void)c;if(id!=17)return 0;*p=target;return 1;}
static SA1Surface none(void*c,float x,float z){(void)c;(void)x;(void)z;return (SA1Surface){0};}
static SA1SurfaceFrame encoded(int x,int y,int z){
    double sx=sin(x*(TAU/65536.)),cx=cos(x*(TAU/65536.));
    double sy=sin(y*(TAU/65536.)),cy=cos(y*(TAU/65536.));
    double sz=sin(z*(TAU/65536.)),cz=cos(z*(TAU/65536.));
    return (SA1SurfaceFrame){.forward={(float)(cz*cy+sz*sx*sy),(float)(sz*cy-cz*sx*sy),(float)(cx*sy)},
        .up={(float)(-sz*cx),(float)(cz*cx),(float)sx},
        .right={(float)(cz*sy-sz*sx*cy),(float)(sz*sy+cz*sx*cy),(float)(-cx*cy)},.valid=1};
}
static void frame_cases(void){
    const int pitch[]={-0x3f00,-0x3000,0,0x1800,0x3f00},roll[]={0,0x2000,0x9000};
    const uint16_t yaw[]={0,1,0x1fff,0x4000,0x8000,0xbfff,0xffff};
    SA1PC2004HomingMath math={.sine=sine};unsigned cases=0;
    for(unsigned i=0;i<5;i++)for(unsigned j=0;j<3;j++)for(unsigned k=0;k<7;k++){
        SA1SurfaceFrame f=encoded(pitch[i],yaw[k],roll[j]);uint16_t recovered;
        assert(sa1_pc2004_task_yaw(&f,&recovered)&&recovered==yaw[k]);
        uint16_t next=(uint16_t)(yaw[k]+0x800);SA1SurfaceFrame expected=encoded(pitch[i],next,roll[j]);
        assert(sa1_pc2004_task_turn(&f,recovered,next,&math));
        assert(near(f.forward.x,expected.forward.x)&&near(f.forward.y,expected.forward.y)&&near(f.forward.z,expected.forward.z));
        assert(near(f.up.x,expected.up.x)&&near(f.up.y,expected.up.y)&&near(f.up.z,expected.up.z));cases++;
    }
    for(int sign=-1;sign<=1;sign+=2)for(unsigned k=0;k<7;k++){
        SA1SurfaceFrame f=encoded(sign*0x4000,yaw[k],0);uint16_t recovered;
        assert(sa1_pc2004_task_yaw(&f,&recovered)&&recovered==yaw[k]);cases++;
    }
    printf("Canonical task-frame inverse/turn: PASS %u independent Rz*Rx*Ry matrix cases including normal poles; source angle history not proven\n",cases);
}
static SA1PlayerState airborne(SA1PhysicsWorld*w){
    SA1PlayerState p;sa1_player_reset(&p,w);p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
    p.pc2004_action_state=8;p.ball_form=true;p.vel=(SA1Vec3){2,3,4};
    p.animation_airborne_request=14;p.animation_airborne_request_valid=true;return p;
}
static void tick(SA1PlayerState*p,SA1PhysicsWorld*w,SA1Input input){sa1_physics_step_steam_sonic_flat(p,&input,0,w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);assert(!p->pc2004_homing_failed);}
int main(void){
    frame_cases();
    SA1PC2004HomingList list={.count=1,.entries={{17,3300}}};
    SA1PC2004HomingServices services={.list=&list,.resolve=resolve,.math={.bearing=bearing,.sine=sine},.first_hit_category=-1};
    SA1PhysicsWorld world={.query_surface=none,.reset_position={0,100,0},.reset_min_x=-10000,.reset_max_x=10000,
        .reset_min_y=-10000,.reset_min_z=-10000,.reset_max_z=10000,.homing=&services};
    target=(SA1PC2004HomingPose){.task_origin={40,89,40}};
    SA1PlayerState p=airborne(&world);tick(&p,&world,(SA1Input){.jump_pressed=true,.jump_held=true});
    assert(p.pc2004_action_state==14&&p.pc2004_air_attack_timer==0&&p.pc2004_air_attack_age==0);
    assert(p.pc2004_homing_calls==2&&p.pc2004_homing_tracks==2&&p.pc2004_homing_target==17);
    /*Both492300 calls precede any position integration: same deltaY=-10,
     * source rawY4000 ->3800 ->3000, with no gravity on either TRACK call.*/
    double vertical=-10./sqrt(3300.),horizontal=sqrt(1-vertical*vertical);
    assert(near(p.vel.x,(float)(cos(0x3000*(TAU/65536.))*horizontal*5)));
    assert(near(p.vel.y,(float)(vertical*5))&&near(p.vel.z,(float)(sin(0x3000*(TAU/65536.))*horizontal*5)));
    assert(near(p.pos.x,p.vel.x)&&near(p.pos.y,100+p.vel.y)&&near(p.pos.z,p.vel.z));
    tick(&p,&world,(SA1Input){0});assert(p.pc2004_homing_calls==1&&p.pc2004_homing_tracks==1&&p.pc2004_air_attack_age==1);
    services.first_hit_category=3;tick(&p,&world,(SA1Input){.jump_held=true});
    assert(p.pc2004_action_state==8&&p.pc2004_air_attack_timer==-1&&p.pc2004_homing_calls==0&&p.jump_ticks==0);
    services.first_hit_category=-1;
    /*An empty live service must preserve the existing empty-target path.*/
    list.count=0;SA1PlayerState live=airborne(&world),legacy=live;SA1PhysicsWorld old=world;old.homing=NULL;
    tick(&live,&world,(SA1Input){.jump_pressed=true});tick(&legacy,&old,(SA1Input){.jump_pressed=true});
    assert(live.pc2004_air_attack_timer==2&&legacy.pc2004_air_attack_timer==2&&live.pc2004_homing_calls==1);
    assert(near(live.vel.x,legacy.vel.x)&&near(live.vel.y,legacy.vel.y)&&near(live.vel.z,legacy.vel.z));
    assert(near(live.pos.x,legacy.pos.x)&&near(live.pos.y,legacy.pos.y)&&near(live.pos.z,legacy.pos.z));
    /*A nonempty list with no eligible facing target executes acceleration in
     * BOTH initialization and movement; a positive timer then locks pursuit.*/
    list.count=1;target.task_origin=(SA1Vec3){0,99,-40};p=airborne(&world);
    tick(&p,&world,(SA1Input){.jump_pressed=true});
    assert(p.pc2004_homing_calls==2&&p.pc2004_homing_tracks==0&&p.pc2004_air_attack_timer==2);
    const SA1SteamPlayerParameters*q=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    float normal=3;for(unsigned i=0;i<2;i++)normal+=-q->weight+normal*q->air_resist_y;
    assert(near(p.vel.y,normal));
    /*The initializer's nonzero post-call timer writes5 even for special
     * players. Only its genuinely empty-list branch writes10.*/
    services.special_player=1;target.task_origin=(SA1Vec3){0,99,-40};p=airborne(&world);
    /*Special bypasses facing, so use an unresolved-range candidate list with
     * a cached distance at the strict10000 selection boundary.*/
    list.entries[0].task_distance_squared=10000;tick(&p,&world,(SA1Input){.jump_pressed=true});
    assert(p.pc2004_homing_calls==2&&!p.pc2004_homing_tracks&&near(p.vel.z,4.404f));
    services.special_player=0;list.entries[0].task_distance_squared=3300;
    target.task_origin=(SA1Vec3){40,109,40};p=airborne(&world);tick(&p,&world,(SA1Input){.jump_pressed=true});
    assert(p.pc2004_homing_calls==2&&p.pc2004_homing_tracks==1&&p.pc2004_air_attack_timer==2);
    assert(near(p.vel.y,-q->weight));
    puts("Public homing handoff: PASS double entry/current target/raw yaw/world velocity, one-call continuation, cached-category cancel, empty-service equivalence and failed-search double acceleration; no retail trace");
}
