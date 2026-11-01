#include "spring_launch.h"
#include <math.h>
static int finite_vec(SA1Vec3 v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
/* Ninja postmultiplies the matrix in Z,X,Y call order. Keep float matrix
 * boundaries, rather than treating the calls as successive vector rotations. */
static void rotate(float m[9],int axis,float s,float c){
    int a=axis==0?1:0,b=axis==2?1:2;
    for(int row=0;row<3;row++){
        float x=m[a*3+row],y=m[b*3+row];
        if(axis==1){m[a*3+row]=(float)((double)x*c-(double)y*s);m[b*3+row]=(float)((double)y*c+(double)x*s);}
        else{m[a*3+row]=(float)((double)x*c+(double)y*s);m[b*3+row]=(float)((double)y*c-(double)x*s);}
    }
}
static SA1Vec3 vector(const SA3DSSpringTrig*t,const int16_t angles[3],float strength,int physical){
    float m[9]={1,0,0,0,1,0,0,0,1};
    const int order[3]={2,0,1};
    for(int i=0;i<3;i++){
        int a=order[i];float s=t->sine[a],c=t->cosine[a];
        /* 4413E0 taskY=4000-SETY, then 43EC90 rotates -taskY. */
        if(a==1&&physical){s=-t->cosine[a];c=t->sine[a];}
        if(angles[a]|| (a==1&&physical))rotate(m,a,s,c);
    }
    return (SA1Vec3){m[3]*strength,m[4]*strength,m[5]*strength};
}
int sa3ds_spring_launch_prepare(const SA3DSMovementPlacement*p,
    const SA3DSSpringTrig*t,SA1Vec3 player,int grounded,float forward,SA3DSSpringLaunch*out){
    if(!out)return 0;
    *out=(SA3DSSpringLaunch){0};
    if(!p||!t||!grounded||!finite_vec(player)||!finite_vec(p->position)||!isfinite(forward))return 0;
    for(int i=0;i<3;i++)if(!isfinite(p->parameters[i])||!isfinite(t->sine[i])||!isfinite(t->cosine[i])||
        fabsf(t->sine[i])>1.f||fabsf(t->cosine[i])>1.f)return 0;
    SA3DSSpringLaunch result={0};
    if(!sa3ds_movement_spring_command(p,&result.command))return 0;
    SA1Vec3 v=vector(t,p->angles,result.command.strength,0);
    result.endpoint=(SA1Vec3){p->position.x+v.x,p->position.y+v.y,p->position.z+v.z};
    result.position=player;
    if(forward>=1.f)result.position=result.endpoint;
    else if((p->type&4095)==1){
        float dx=result.endpoint.x-player.x,dz=result.endpoint.z-player.z;
        float distance=(float)sqrt((double)dx*dx+(double)dz*dz);
        if(!isfinite(distance))return 0;
        if(distance>=2.f&&distance<14.f){
            result.position.x=(float)((double)player.x+(double)dx*(1.0/(double)distance)*2.0);
            result.position.z=(float)((double)player.z+(double)dz*(1.0/(double)distance)*2.0);
        }
    }else{
        result.position=(SA1Vec3){result.endpoint.x,(float)((double)p->position.y+(double)v.y*0.8f),result.endpoint.z};
    }
    result.world_velocity=v;
    if((p->type&4095)==2){
        result.world_velocity=vector(t,p->angles,result.command.strength,1);
        result.local_velocity=(SA1Vec3){0,result.command.strength,0};
        result.task_angles[0]=p->angles[0];
        result.task_angles[1]=0x4000-(int32_t)p->angles[1];
        result.task_angles[2]=p->angles[2];
        result.angles_written=result.local_velocity_written=1;
    }
    if(!finite_vec(result.endpoint)||!finite_vec(result.position)||!finite_vec(result.world_velocity))return 0;
    *out=result;return 1;
}
