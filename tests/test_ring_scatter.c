#include "ring_scatter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static unsigned random_calls,floor_calls;
static uint16_t random15(void*c){(void)c;random_calls++;return 15;}
static double sine(void*c,uint16_t a){(void)c;return sin(a*(6.2831853071795864769/65536.));}
static int32_t bearing(void*c,double y,double x){(void)c;return (int32_t)trunc(atan2(y,x)*65536.*.15915497620314795);}
static double floor_query(void*c,SA1Vec3 p,int32_t a[3]){(void)c;(void)p;floor_calls++;a[0]=a[2]=0;return 0;}
static SA1Vec3 tilt(void*c,const int32_t a[3],SA1Vec3 v){
    (void)c;assert(a[0]==0);if((uint16_t)a[2]==0x4000)return (SA1Vec3){-v.y,v.x,v.z};
    assert(a[2]==0);return v;
}
static SA3DSRingScatterServices services={.random15=random15,.sine=sine,.atan_bams=bearing,.floor=floor_query,.tilt=tilt};
static SA3DSRingScatterInput input;
static SA3DSRingScatterResult result;
static void tick(SA3DSRingScatter*a,int32_t*p){assert(sa3ds_ring_scatter_tick(a,p,&input,&services,&result));}
int main(void){
    int32_t phase=0;SA3DSRingScatter a={0};
    tick(&a,&phase);assert(a.action==1&&a.collision_id==11&&a.position.y==10&&a.cached_floor==-100);
    assert(phase==1&&a.floor_phase==0&&a.age==0&&a.spin==30&&result.initialize_collision&&random_calls==1);
    tick(&a,&phase);assert(floor_calls==1&&a.floor_phase==8&&a.age==1&&a.spin==35);
    assert(fabsf(a.position.y-11.422222f)<2e-6f&&fabsf(a.position.z-.3f)<1e-7f&&result.normal_draw&&!result.enqueue_collision);
    for(unsigned n=2;n<=47;n++){tick(&a,&phase);assert(result.enqueue_collision==(n==47));}
    assert(a.collection_delay==47);
    unsigned updates=47,bounces=0;float scale=a.scale_or_floor;
    while(a.action==1&&updates<2000){tick(&a,&phase);if(a.action==1&&a.scale_or_floor<scale){bounces++;scale=a.scale_or_floor;}updates++;}
    assert(a.action==2&&updates<2000&&a.radial_x==0&&a.radial_z==0&&a.scale_or_floor==0);
    /*The transition retains the last parabola age. State2's source counter
     * need not start at0; use the exact boundary as a distinct case.*/
    a.age=98;input.global_frame=0;SA1Vec3 pos=a.position;tick(&a,&phase);
    assert(a.age==99&&a.action==2&&!result.normal_draw&&result.enqueue_collision&&result.display_callback==2);
    assert(a.position.x==pos.x&&a.position.y==pos.y&&a.position.z==pos.z);
    input.global_frame=2;tick(&a,&phase);assert(a.action==3&&result.normal_draw&&a.age==100);
    tick(&a,&phase);assert(result.destroy);
    for(unsigned n=0;n<10;n++){a=(SA3DSRingScatter){0};tick(&a,&phase);}
    assert(phase==2&&random_calls==11);
    /*Mode1 verifies cached primary-owner identity; mode2's source branch
     * omits that test. A dead primary still suppresses either award.*/
    input=(SA3DSRingScatterInput){.hit=1};a=(SA3DSRingScatter){.action=1};tick(&a,&phase);
    assert(a.action==4&&result.reset_pickup_effect&&result.clear_hit&&!result.award_ring&&result.display_callback==0);
    a=(SA3DSRingScatter){.action=2};tick(&a,&phase);assert(result.award_ring&&result.sound7&&result.display_callback==3);
    input.primary_dead=1;a=(SA3DSRingScatter){.action=2};tick(&a,&phase);assert(!result.award_ring);
    input=(SA3DSRingScatterInput){.hit=1,.hit_is_primary_player=1};a=(SA3DSRingScatter){.action=1};tick(&a,&phase);assert(result.award_ring);
    input=(SA3DSRingScatterInput){.paused=1};float spin=a.spin;tick(&a,&phase);assert(a.spin==spin&&result.pickup_effect);
    input=(SA3DSRingScatterInput){.render_suppressed=1};for(unsigned n=0;n<30;n++)tick(&a,&phase);
    assert(a.action==4&&!result.pickup_effect);input.render_suppressed=0;tick(&a,&phase);assert(a.action==3&&result.pickup_effect);
    /*Low scale always transitions this tick, including the middle decay
     * interval. It does not wait until a synthetic multi-tick timer expires.*/
    for(unsigned n=0;n<3;n++){
        a=(SA3DSRingScatter){.action=1,.scale_or_floor=.19f,.decay_age=n==0?1:n==1?3:13,.floor_phase=8,.cached_floor=4};
        tick(&a,&phase);assert(a.action==2&&a.scale_or_floor==4&&!result.enqueue_collision);
        assert(a.radial_x==0&&a.radial_z==0);
    }
    a=(SA3DSRingScatter){.action=1,.angles={0,0,0x4000},.scale_or_floor=1,.launch_y=4.6f,.floor_phase=8};
    tick(&a,&phase);assert(a.angles[1]==bearing(NULL,-1,.3f));
    a=(SA3DSRingScatter){.action=1,.angles={0,0,0x4000},.scale_or_floor=.19f,.decay_age=13,.floor_phase=8,.cached_floor=4};
    tick(&a,&phase);assert(a.action==2&&a.radial_x==-6.880000114440918f&&a.radial_z==0);
    printf("Scatter task: PASS init phase/RNG,47-update CCL delay,parabola/bounce lifecycle (%u updates,%u later bounces),immediate low-scale transition,100-counter blink/delete,phase-specific attribution and pickup suppression/pause\n",updates,bounces);
}
