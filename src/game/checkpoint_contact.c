#include "checkpoint_contact.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static int rectangle(const SA3DSCheckpointSweep*s,SA1Vec3 p,float radius){
    float a=(float)((double)s->box_x+radius),b=(float)((double)s->box_z+radius);
    float sn=(float)s->sine(s->context,s->box_yaw),cs=(float)s->sine(s->context,(uint16_t)(s->box_yaw+16384));
    if(!isfinite(sn)||!isfinite(cs))return -1;
    const float x[4]={a,a,-a,-a},z[4]={b,-b,-b,b};SA1Vec3 corners[4];
    for(unsigned i=0;i<4;i++){
        float rx=(float)((double)x[i]*cs+(double)z[i]*sn);
        float rz=(float)((double)z[i]*cs-(double)x[i]*sn);
        corners[i]=(SA1Vec3){(float)((double)s->box_center.x+rx),p.y,(float)((double)s->box_center.z+rz)};
    }
    for(unsigned i=0;i<4;i++){
        unsigned j=(i+1)&3;
        float ax=(float)((double)corners[i].x-p.x),az=(float)((double)corners[i].z-p.z);
        float bx=(float)((double)corners[j].x-p.x),bz=(float)((double)corners[j].z-p.z);
        float normal_y=(float)((double)az*bx-(double)ax*bz);
        if(!(normal_y>0))return 0;
    }
    return 1;
}
int sa3ds_checkpoint_sweep(const SA3DSCheckpointSweep*s,SA3DSCheckpointContact*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!s||!s->sine||!finite3(s->previous_task)||!finite3(s->current_task)||!finite3(s->box_center)||
       !isfinite(s->player_radius)||!isfinite(s->player_half_height)||!isfinite(s->player_foot_radius)||
       !isfinite(s->box_x)||!isfinite(s->box_y)||!isfinite(s->box_z)||
       s->player_radius<0||s->player_half_height<0||s->player_foot_radius<0||s->box_x<0||s->box_y<0||s->box_z<0)return 0;
    double dx=(float)((double)s->current_task.x-s->previous_task.x);
    double dy=(float)((double)s->current_task.y-s->previous_task.y);
    double dz=(float)((double)s->current_task.z-s->previous_task.z);
    double distance=sqrt((float)(dz*dz+dy*dy+dx*dx));
    SA1Vec3 step={0};
    if(distance<=0)distance=(double).01f;
    else{
        step.x=(float)(dx/distance*2.5);
        step.y=(float)(dy/distance);step.y=(float)((double)step.y*2.5);
        step.z=(float)(dz/distance);step.z=(float)((double)step.z*2.5);
    }
    double count=ceil(distance*(double).4f);
    if(!isfinite(count)||count<1||count>2147483647.)return 0;
    uint32_t samples=(uint32_t)count;out->samples=samples;
    SA1Vec3 point=s->previous_task;point.y=(float)((double)point.y+s->player_half_height);
    float upper=(float)((double)s->box_center.y+s->box_y),lower=(float)((double)s->box_center.y-s->box_y);
    for(uint32_t i=0;i<samples;i++){
        if(i+1==samples){point=s->current_task;point.y=(float)((double)point.y+s->player_half_height);}
        else {point.x=(float)((double)point.x+step.x);point.y=(float)((double)point.y+step.y);point.z=(float)((double)point.z+step.z);}
        float top=(float)((double)point.y+s->player_half_height),bottom=(float)((double)point.y-s->player_half_height);
        unsigned kind=0;int inside;
        if(bottom<upper&&(double)upper-4.<bottom){
            inside=rectangle(s,point,s->player_foot_radius);if(inside<0)return 0;if(inside)kind=1;
        }
        if(!kind){
            inside=rectangle(s,point,s->player_radius);if(inside<0)return 0;
            if(inside){
                if(lower<top&&(double)top<(double)lower+2.)kind=3;
                else if((double)bottom<(double)upper-4.&&lower<top)kind=2;
            }
        }
        if(kind){out->kind=(uint8_t)kind;out->sample_index=i;out->sample=point;return 1;}
    }
    return 1;
}
static int push(const SA3DSCCLBody*self,const SA3DSCCLShape*own,const SA3DSCCLBody*other,const SA3DSCCLShape*theirs){
    return (self->rules.push_receive&theirs->push)&&
        !((own->attributes&0x4000)&&(theirs->attributes&0x4000)&&(theirs->damage&0x10))&&
        !(other->rules.push_inhibit&own->push);
}
int sa3ds_checkpoint_record_contact(SA3DSCCLBody*p,SA3DSCCLShape*primary,
    SA3DSCCLBody*c,SA3DSCCLShape*box,const SA3DSCheckpointContact*hit){
    if(!p||!primary||!c||!box||!hit||hit->kind>3||primary->form!=7||box->form!=3)return 0;
    if(!hit->kind)return 1;
    if(push(c,box,p,primary)||push(p,primary,c,box))return 0; /*supported profile inhibits both*/
    if(!sa3ds_ccl_record_contact(p,primary,0,c,box,0))return 0;
    if(hit->kind==1){c->status|=0x10;p->outer_flags|=8;c->outer_flags|=2;}
    else if(hit->kind==3){c->status|=0x20;p->outer_flags|=2;c->outer_flags|=8;}
    else{p->outer_flags|=4;c->outer_flags|=4;}
    return 1;
}
