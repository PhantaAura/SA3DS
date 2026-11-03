#include "checkpoint_player.h"
#include "course_clock.h"
#include "../src/sa1/physics/movement.h"
#include "../src/sa1/physics/surface_frame.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    SA3DSCourseClock c;sa3ds_course_clock_reset(&c);
    for(unsigned i=0;i<3600;i++)sa3ds_course_clock_tick(&c);
    assert(c.time[0]==1&&!c.time[1]&&!c.time[2]);
    c.enabled=0;sa3ds_course_clock_tick(&c);assert(c.updates==3601&&c.time[0]==1&&!c.time[2]);
    c.enabled=1;c.time[0]=99;c.time[1]=c.time[2]=59;sa3ds_course_clock_tick(&c);assert(c.time[0]==99&&c.time[1]==59&&c.time[2]==59);
    c.time[2]=255;sa3ds_course_clock_tick(&c);assert(c.time[2]==0); /*source raw signed byte*/
    SA1PhysicsWorld world={0};SA1PlayerState p;sa1_player_reset(&p,&world);
    p.pos=(SA1Vec3){286,2,740};p.ground_normal=(SA1Vec3){0,1,0};p.yaw=0;p.vel=(SA1Vec3){0,0,2.5};
    int16_t spin;assert(sa3ds_checkpoint_player_spin(&p,0,&spin)&&spin==4551);
    p.vel.z=-2.5;assert(sa3ds_checkpoint_player_spin(&p,0,&spin)&&spin==4551); /*source facing sign, not velocity sign*/
    p.yaw=3.14159265358979323846f;assert(sa3ds_checkpoint_player_spin(&p,0,&spin)&&spin==-4551);
    SA1SurfaceFrame f=sa1_surface_frame_make((SA1Vec3){.1f,.98f,.05f},(SA1Vec3){.2f,0,1});
    p.ground_normal=f.up;p.surface_forward=f.forward;p.surface_right=f.right;p.surface_frame_active=true;
    SA1Vec3 foot=sa1_vec3_sub(p.pos,sa1_vec3_normalize(p.ground_normal));SA3DSRestartRecord r;
    uint8_t time[3]={2,17,43};assert(sa3ds_checkpoint_capture_player(&r,&p,0,time));
    assert(r.task_position.x==foot.x&&r.task_position.y==foot.y&&r.task_position.z==foot.z&&r.frames==43);
    p.pos=(SA1Vec3){-999,0,0};assert(sa3ds_checkpoint_restore_player(&r,&p,&world,NULL,NULL));
    SA1Vec3 restored=sa1_vec3_sub(p.pos,p.ground_normal);
    assert(sa1_vec3_length(sa1_vec3_sub(restored,foot))<.00001f);
    assert(sa1_vec3_dot(p.ground_normal,f.up)>.99999f&&sa1_vec3_dot(p.surface_forward,f.forward)>.99999f);
    assert(p.grounded&&p.vel.x==0&&p.vel.y==0&&p.vel.z==0&&p.surface_id==-1);
    puts("Checkpoint player/clock: PASS source foot bridge,canonical sloped frame restore,facing-signed spin,60Hz rollover/cap/disable/raw byte; original task angle history separate");
}
