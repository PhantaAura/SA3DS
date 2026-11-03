#include "restart_record.h"
#include <string.h>
void sa3ds_restart_record_clear(SA3DSRestartRecord*r){memset(r,0,sizeof(*r));}
uint32_t sa3ds_restart_record_check(SA3DSRestartRecord*r,int16_t level,int16_t act){
    uint32_t current=((uint32_t)(int32_t)level<<8)|(uint32_t)(int32_t)act;
    if(current!=(uint32_t)r->stage_act)sa3ds_restart_record_clear(r);
    return r->valid;
}
void sa3ds_restart_record_store(SA3DSRestartRecord*r,int16_t level,int16_t act,
    SA1Vec3 position,const int32_t angles[3],const uint8_t time[3]){
    r->stage_act=(uint16_t)(((uint32_t)(int32_t)level<<8)|(uint32_t)(int32_t)act);
    r->minutes=time[0];r->seconds=time[1];r->frames=time[2];
    memcpy(&r->task_position,&position,sizeof(position));
    memcpy(r->task_angles,angles,sizeof(r->task_angles));r->valid=1;
}
void sa3ds_restart_record_restore(const SA3DSRestartRecord*r,
    SA1Vec3*position,int32_t angles[3],uint8_t time[3]){
    memcpy(position,&r->task_position,sizeof(*position));
    memcpy(angles,r->task_angles,sizeof(r->task_angles));
    time[0]=r->minutes;time[1]=r->seconds;time[2]=r->frames;
}
