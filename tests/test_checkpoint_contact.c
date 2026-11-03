#include "checkpoint_contact.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static double sine(void*context,uint16_t a){
    (void)context;if(!(a&16383)){static const int values[]={0,1,0,-1};return values[a>>14];}
    return sin(a*(6.2831853071795864769/65536.));
}
static SA3DSCheckpointContact stationary(SA3DSCheckpointSweep*s,float x,float y,float z){
    s->previous_task=s->current_task=(SA1Vec3){x,y,z};SA3DSCheckpointContact hit;
    assert(sa3ds_checkpoint_sweep(s,&hit)&&hit.samples==1);return hit;
}
int main(void){
    SA3DSCheckpointSweep s={.box_center={0,6,0},.player_radius=4,.player_half_height=4,.player_foot_radius=.65f,
        .box_x=12,.box_y=4,.box_z=.5f,.sine=sine};
    SA3DSCheckpointContact h=stationary(&s,0,0,0);assert(h.kind==2);
    assert(!stationary(&s,0,0,10).kind); /*sphere substitute would falsely activate*/
    assert(stationary(&s,15.9f,0,4.4f).kind==2); /*source expanded rectangle, not rounded*/
    assert(!stationary(&s,16,0,0).kind&&!stationary(&s,0,0,4.5f).kind);
    assert(stationary(&s,nextafterf(16,0),0,0).kind==2);
    assert(stationary(&s,0,8,0).kind==1);
    assert(!stationary(&s,0,8,2).kind); /*top branch uses the .65 foot radius*/
    assert(!stationary(&s,0,6,0).kind); /*strict source four-unit boundary*/
    assert(stationary(&s,0,-5,0).kind==3);
    assert(!stationary(&s,0,-6,0).kind&&!stationary(&s,0,10,0).kind);
    s.previous_task=(SA1Vec3){0,0,-12};s.current_task=(SA1Vec3){0,0,12};
    assert(sa3ds_checkpoint_sweep(&s,&h)&&h.kind==2&&h.sample_index==3&&h.samples==10&&h.sample.z==-2);
    s.previous_task=(SA1Vec3){0,0,100};s.current_task=(SA1Vec3){0,0,102.5f};
    assert(sa3ds_checkpoint_sweep(&s,&h)&&!h.kind&&h.samples==2); /*source .4f, not .4double*/
    s.box_yaw=16384;assert(stationary(&s,0,0,15).kind==2);assert(!stationary(&s,5,0,0).kind);
    s.box_yaw=0;
    SA3DSCCLBody player,checkpoint;sa3ds_ccl_body_init(&player,1);sa3ds_ccl_body_init(&checkpoint,2);
    player.rules=(SA3DSCCLRules){1,16,32,0x40000};checkpoint.rules=(SA3DSCCLRules){4,64,128,0x100000};
    SA3DSCCLShape primary={.form=7,.push=6,.damage=0xe0,.attributes=0x420},box={.form=3,.push=0xb0,.damage=0xe0};
    for(unsigned kind=1;kind<=3;kind++){
        sa3ds_ccl_body_clear(&player,&primary,1);sa3ds_ccl_body_clear(&checkpoint,&box,1);
        h=(SA3DSCheckpointContact){.kind=(uint8_t)kind};
        assert(sa3ds_checkpoint_record_contact(&player,&primary,&checkpoint,&box,&h));
        assert((checkpoint.outer_flags&1)&&checkpoint.cached_other_id==1&&checkpoint.cached_other_shape==0);
        assert(player.cached_other_id==2&&checkpoint.hit_count==1&&player.hit_count==1);
        assert(!(player.status&4));
        assert(checkpoint.outer_flags&(kind==1?2:kind==3?8:4));
    }
    player.rules.push_inhibit=0;
    assert(!sa3ds_checkpoint_record_contact(&player,&primary,&checkpoint,&box,&h));
    s.box_x=NAN;assert(!sa3ds_checkpoint_sweep(&s,&h));
    puts("Checkpoint contact: PASS source7/3 thin-box sweep,strict side/top/underside boundaries,ULP edges,yaw,2.5-step crossing,.4f sample count,category4 contact cache/status and inhibited-push gate; native scheduling separate");
}
