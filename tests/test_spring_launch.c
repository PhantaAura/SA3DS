#include "../src/game/spring_launch.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(float a,float b){assert(fabsf(a-b)<0.00002f);}
static void vec(SA1Vec3 a,float x,float y,float z){near(a.x,x);near(a.y,y);near(a.z,z);}
int main(void){
    SA3DSMovementPlacement p={.type=2,.position={10,20,30},.parameters={12.9f,0,0}};
    SA3DSSpringTrig t={{0,0,0},{1,1,1}};
    SA3DSSpringLaunch out;
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){100,200,300},1,0,&out));
    vec(out.endpoint,10,25,30);vec(out.position,10,24,30);vec(out.world_velocity,0,5,0);
    assert(out.command.command==4&&out.command.sonic_action==10&&out.command.animation_request==16);
    assert(out.command.control_timer==12&&out.angles_written&&out.local_velocity_written);
    assert(out.task_angles[1]==0x4000);
    vec(out.local_velocity,0,5,0);
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,1,&out));
    vec(out.position,10,25,30); /* inclusive local-forward boundary */
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,nextafterf(1,0),&out));
    vec(out.position,10,24,30);
    /* Source retains the .8 product until addition, then stores one float.
     * An extra float store of the product changes this fixture by one ULP. */
    p.position.y=9.844924926757812f;p.parameters[1]=5.779812812805176f;
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));
    assert(out.position.y==18.468774795532227f);
    p.position.y=20;p.parameters[1]=0;
    p.angles[2]=0x4000;t.sine[2]=1;t.cosine[2]=0;
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,1,&out));
    vec(out.endpoint,5,20,30);vec(out.world_velocity,-5,0,0);
    /* Independent tilted analytic result: Rz(45)*Rx(45)*Ry(90)*Y.
     * Yaw cannot change the local Y input; reversing vector-call order would. */
    p.angles[0]=p.angles[2]=0x2000;p.angles[1]=0x4000;
    t=(SA3DSSpringTrig){{0.7071067811865475f,1,0.7071067811865475f},
                         {0.7071067811865475f,0,0.7071067811865475f}};
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));
    vec(out.world_velocity,-2.5f,2.5f,3.53553390593f);
    vec(out.endpoint,7.5f,22.5f,33.53553390593f);
    vec(out.position,7.5f,22,33.53553390593f);assert(out.task_angles[1]==0);
    p=(SA3DSMovementPlacement){.type=1};t=(SA3DSSpringTrig){{0,0,0},{1,1,1}};
    const float distances[]={nextafterf(2,0),2,13.5f,14,nextafterf(14,INFINITY)};
    for(unsigned i=0;i<sizeof(distances)/sizeof(*distances);i++){
        float d=distances[i];assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){-d,7,0},1,0,&out));
        vec(out.position,d>=2&&d<14?-d+2:-d,7,0);
        assert(out.command.command==3&&out.command.sonic_action==9&&!out.angles_written&&!out.local_velocity_written);
    }
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){-6,7,-8},1,0,&out));
    vec(out.position,-4.8f,7,-6.4f); /* normalize XZ together */
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){-6,7,-8},1,1,&out));vec(out.position,0,5,0);
    p.type=2;p.parameters[0]=-2.5f;
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));assert(out.command.control_timer==0);
    p.parameters[0]=65535.9f;
    assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));assert(out.command.control_timer==-1);
    assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},0,1,&out));assert(out.command.command==0);
    assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){NAN,0,0},1,1,&out));
    assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,INFINITY,&out));
    t.sine[0]=NAN;assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));t.sine[0]=0;
    p.parameters[2]=NAN;assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));p.parameters[2]=0;
    p.position.y=INFINITY;assert(!sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0,0,0},1,0,&out));
    puts("Spring launch preparation: PASS (grounded source branch, ZXY frame, placement boundaries and command/timer; no physics application)");
}
