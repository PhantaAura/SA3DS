#include "restart_record.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    SA3DSRestartRecord r,zero={0};memset(&r,0xa5,sizeof(r));
    sa3ds_restart_record_clear(&r);assert(!memcmp(&r,&zero,sizeof(r)));
    assert(!sa3ds_restart_record_check(&r,1,0));
    /*Capture actual task payload, not checkpoint SET position or just yaw.*/
    SA1Vec3 p={286.25f,-0.f,742.5f},restored;
    int32_t angles[3]={-256,0x1234abcd,-821},out[3];
    uint8_t time[3]={3,27,59},clock[3];
    sa3ds_restart_record_store(&r,1,0,p,angles,time);
    SA3DSRestartRecord before=r;
    assert(sa3ds_restart_record_check(&r,1,0)==1);
    assert(!memcmp(&r,&before,sizeof(r)));
    sa3ds_restart_record_restore(&r,&restored,out,clock);
    assert(!memcmp(&restored,&p,sizeof(p))&&!memcmp(out,angles,sizeof(out)));
    assert(!memcmp(clock,time,sizeof(clock)));
    /*Repeated deaths reuse the captured time, not elapsed time or zero.*/
    memset(clock,0,sizeof(clock));sa3ds_restart_record_restore(&r,&restored,out,clock);
    assert(clock[0]==3&&clock[1]==27&&clock[2]==59);
    assert(!sa3ds_restart_record_check(&r,1,1));assert(!memcmp(&r,&zero,sizeof(r)));
    time[0]=255;time[1]=128;time[2]=255;
    sa3ds_restart_record_store(&r,1,1,p,angles,time);
    sa3ds_restart_record_restore(&r,&restored,out,clock);
    assert(!memcmp(time,clock,sizeof(time))); /*raw source bytes, no invented clamp*/
    assert(sa3ds_restart_record_check(&r,1,1));
    assert(!sa3ds_restart_record_check(&r,2,1));
    sa3ds_restart_record_store(&r,-1,0,p,angles,time);
    assert(!sa3ds_restart_record_check(&r,-1,0)); /*saved16 vs full signed expression*/
    sa3ds_restart_record_store(&r,1,-1,p,angles,time);
    assert(!sa3ds_restart_record_check(&r,1,-1));
    puts("Restart record: PASS full task position/rotation/raw clock, repeated restore, act/level invalidation and signed key comparison; native checkpoint activation remains separate");
}
