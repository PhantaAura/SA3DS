#include "pc2004_homing.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {SA1PC2004HomingPose poses[4];unsigned resolves,randoms;uint16_t random,bearing;int forced;} Context;
static int resolve(void*ctx,uint32_t id,SA1PC2004HomingPose*out){Context*c=ctx;c->resolves++;if(!id||id>3)return 0;*out=c->poses[id];return 1;}
static uint16_t bearing(void*ctx,float z,float x){Context*c=ctx;return c->forced?c->bearing:(uint16_t)(int32_t)(atan2((double)z,x)*65536.*0.15915497620314795);}
static double sine(void*ctx,uint16_t angle){(void)ctx;return sin((double)angle*6.28318530717958647692/65536.);}
static uint16_t random15(void*ctx){Context*c=ctx;c->randoms++;return c->random;}
static int near(float a,float b){return fabsf(a-b)<.00001f;}
static float length(SA1Vec3 v){return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);}
int main(void){
    SA1PC2004HomingList list={0};SA1Vec3 origin={0},target={100,0,0};
    assert(!sa1_pc2004_homing_collect(&list,0,0,0x40,0,1,origin,target));
    target.x=99;assert(sa1_pc2004_homing_collect(&list,0,0,0x40,0,1,origin,target)==1);
    assert(list.count==1&&list.entries[0].task_distance_squared==9801);
    assert(!sa1_pc2004_homing_collect(&list,0,0,0x40,11,2,origin,target));
    assert(!sa1_pc2004_homing_collect(&list,0,0,0,0,2,origin,target));
    assert(!sa1_pc2004_homing_collect(&list,0,2,0x40,0,2,origin,target));
    assert(!sa1_pc2004_homing_collect(&list,-1,0,0x40,0,2,origin,target));
    sa1_pc2004_homing_reset(&list);target.x=1;
    for(unsigned i=0;i<SA1_PC2004_HOMING_CAPACITY;i++)assert(sa1_pc2004_homing_collect(&list,1,0,0x40,0,1,origin,target)==1);
    assert(!sa1_pc2004_homing_collect(&list,1,0,0x40,0,1,origin,target));
    sa1_pc2004_homing_reset(&list);
    Context c={0};c.poses[1]=(SA1PC2004HomingPose){{20,0,0},{80,-10,0},0};
    c.poses[2]=(SA1PC2004HomingPose){{30,0,0},{-29,-2,0},0};
    assert(sa1_pc2004_homing_collect(&list,0,0,0x40,0,1,origin,c.poses[1].task_origin)==1);
    assert(sa1_pc2004_homing_collect(&list,0,0,0x40,0,2,origin,c.poses[2].task_origin)==1);
    SA1PC2004HomingInput in={.list=&list,.player_task_origin={0},.local_speed={7,2,3},
        .resolve=resolve,.resolve_context=&c,.math={&c,bearing,sine,random15}};
    SA1PC2004AirAttack state={14,0x500,14,-1,0};SA1PC2004HomingResult r;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_TRACK&&r.selected_id==1&&r.next.timer==0);
    /* Cached task distance wins over much nearer current collider center. */
    assert(r.world_velocity.x>4.9f&&r.world_velocity.y<0);
    list.entries[0]=(SA1PC2004HomingCandidate){2,400};list.entries[1]=(SA1PC2004HomingCandidate){1,400};
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.selected_id==2);
    list.count=1;list.entries[0]=(SA1PC2004HomingCandidate){1,400};
    c.poses[1].first_center=(SA1Vec3){0,-10,10};c.poses[1].first_shape_flags=0x20;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.task_yaw==0x800&&near(r.world_velocity.y,-3.535534f));
    assert(r.world_velocity.x>3&&r.world_velocity.z>0&&r.world_velocity.z<1);
    c.poses[1].first_center=(SA1Vec3){10,10,0};
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.next.timer==1&&r.world_velocity.y==0&&near(r.world_velocity.x,5));
    unsigned previous_resolves=c.resolves;state.timer=1;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_DECAY_ACCEL&&r.next.timer==2);
    assert(near(r.local_speed.forward,6.3f)&&r.local_speed.normal==2&&r.local_speed.side==3&&c.resolves==previous_resolves);
    state.timer=-1;c.poses[1].first_center=(SA1Vec3){-10,0,0};
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_ACCEL_ONLY&&r.next.timer==1&&r.local_speed.forward==7);
    in.special_player=1;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_TRACK&&r.task_yaw==0xf000&&near(length(r.world_velocity),10));
    in.special_player=0;c.forced=1;c.bearing=0x5000;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.selected_id==1);
    c.bearing=0x5001;assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_ACCEL_ONLY);
    assert(sa1_pc2004_homing_angle_distance(0,0x8000)==0x8000);c.forced=0;
    c.poses[1]=(SA1PC2004HomingPose){{10,0,0},{0,-1,0},0};list.entries[0].task_distance_squared=100;
    c.poses[3]=c.poses[1];in.partner_id=3;state.timer=1;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.selected_id==3&&r.mode==SA1_PC2004_HOMING_TRACK);
    /* Partner is seeded first and retains an equal cached/task distance tie. */
    in.partner_id=0;state.timer=0;state.age=181;c.random=0;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&near(length(r.world_velocity),3.5f)&&c.randoms==1);
    c.random=32767;assert(sa1_pc2004_homing_step(&state,&in,&r)&&near(length(r.world_velocity),3.9999847f)&&c.randoms==2);
    in.math.random15=0;assert(!sa1_pc2004_homing_step(&state,&in,&r));
    list.count=0;state.timer=0;
    assert(sa1_pc2004_homing_step(&state,&in,&r)&&r.mode==SA1_PC2004_HOMING_DECAY_ACCEL&&r.next.timer==1);
    puts("PC2004 homing: PASS task-range/list order/ring exclusion/cap, cached distance/current centers, angle boundary/turn cap, upward entry rule, partner tie, injected source RNG; kernel-only checks; controller/native verification is separate");
}
