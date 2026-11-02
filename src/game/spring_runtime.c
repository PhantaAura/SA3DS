#include "spring_runtime.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa3ds_spring_query_player_hit(const SA3DSSpringHitEntry *entries,
    unsigned count,unsigned start,SA3DSSpringHitResult *out){
    if(!entries||!out||count>16||start>16)return 0;
    for(unsigned i=start;i<count;i++){
        const SA3DSSpringHitEntry *e=&entries[i];
        if(e->other_shape==-1)break;
        if(e->task_present&&e->ccl_present&&e->ccl_category==0){
            *out=(SA3DSSpringHitResult){i,e->player_index};return 1;
        }
    }
    return 0;
}
void sa3ds_spring_actor_reset(SA3DSSpringActor *a){if(a)memset(a,0,sizeof(*a));}
int sa3ds_spring_actor_tick(SA3DSSpringActor *a,const SA3DSMovementPlacement *p,
    const SA3DSSpringTrig *trig,const SA3DSSpringTickInput *in,SA3DSSpringTickResult *out){
    if(!out)return 0;
    *out=(SA3DSSpringTickResult){0};
    if(!a||!p||!in||((p->type&4095)!=1&&(p->type&4095)!=2)||
       a->action>2||a->cooldown>50||a->player_index>1||
       in->collision_player < -1||in->collision_player>1)return 0;
    if(in->skip_update)return 1;
    SA3DSSpringActor next=*a;
    SA3DSSpringTickResult result={0};
    if(next.action==0){
        /* Allocation initializes work bytes; action0 initializes CCL and
         * returns without enqueue or display. No guessed load distance here. */
        next.action=1;next.cooldown=(p->type&4095)==2?10:0;
    }else if(next.action==1){
        result.draw=1;
        if(!in->paused){
            next.cooldown++;
            if(next.cooldown>10){
                if(in->collision_player>=0){
                    next.action=2;next.player_index=(uint8_t)in->collision_player;
                }else result.enqueue_collision=1;
            }
            if(next.cooldown>50)next.cooldown=50;
        }
    }else{
        /* State2 executes on the FOLLOWING object tick. Source does not pass
         * this state through the active-state global-pause check. */
        if(!sa3ds_spring_launch_prepare(p,trig,in->player_task_origin,
            in->player_grounded,in->player_local_forward,&result.launch))return 0;
        result.launched=result.draw=1;result.player_index=next.player_index;
        next.action=1;next.cooldown=(p->type&4095)==2?5:0;
    }
    *a=next;*out=result;return 1;
}
static void spring_matrix_rotate(float m[9],int axis,float sine,float cosine){
    int a=axis==0?1:0,b=axis==2?1:2;
    for(int row=0;row<3;row++){
        float x=m[a*3+row],y=m[b*3+row];
        if(axis==1){
            m[a*3+row]=(float)((double)x*cosine-(double)y*sine);
            m[b*3+row]=(float)((double)y*cosine+(double)x*sine);
        }else{
            m[a*3+row]=(float)((double)x*cosine+(double)y*sine);
            m[b*3+row]=(float)((double)y*cosine-(double)x*sine);
        }
    }
}
int sa3ds_springb_physical_frame(const SA3DSMovementPlacement *p,
    const SA3DSSpringTrig *trig,SA1SurfaceFrame *out){
    if(!out)return 0;
    *out=(SA1SurfaceFrame){0};
    if(!p||!trig||(p->type&4095)!=2)return 0;
    for(int i=0;i<3;i++)if(!isfinite(trig->sine[i])||!isfinite(trig->cosine[i])||
        fabsf(trig->sine[i])>1||fabsf(trig->cosine[i])>1)return 0;
    float m[9]={1,0,0,0,1,0,0,0,1};
    if(p->angles[2])spring_matrix_rotate(m,2,trig->sine[2],trig->cosine[2]);
    if(p->angles[0])spring_matrix_rotate(m,0,trig->sine[0],trig->cosine[0]);
    /* 4413E0 writes taskY=4000-SETY; 43EC90 uses -taskY. At taskY0
     * the source skips its Y rotation entirely (avoid analytic tiny cos90). */
    if((uint16_t)p->angles[1]!=0x4000u)
        spring_matrix_rotate(m,1,-trig->cosine[1],trig->sine[1]);
    *out=(SA1SurfaceFrame){.up={m[3],m[4],m[5]},.forward={m[0],m[1],m[2]},
        .right={-m[6],-m[7],-m[8]},.valid=1};
    return 1;
}
int sa3ds_spring_command_consume(const SA3DSSpringCommand *c,uint16_t status,
    int16_t prior_timer,SA3DSSpringCommandResult *out){
    if(!out)return 0;
    *out=(SA3DSSpringCommandResult){0};
    if(!c||!isfinite(c->strength)||!(status&0x1000)||
       (c->command!=3&&c->command!=4))return 0;
    if(c->command==3)status&=(uint16_t)~0x2500u;
    else{
        if(status&0x100)status&=(uint16_t)~0x500u;
        status&=(uint16_t)~0x2000u;
    }
    *out=(SA3DSSpringCommandResult){status,16,
        c->command==4?c->control_timer:prior_timer,(uint8_t)(c->command==3?9:10)};
    return 1;
}
int sa3ds_spring_action_select(uint8_t action,uint16_t animation,float world_y,
    float local_y,const SA1SteamPlayerParameters *p,SA3DSSpringActionResult *out){
    if(!out)return 0;
    *out=(SA3DSSpringActionResult){0};
    if(!p||(action!=9&&action!=10)||!isfinite(world_y)||!isfinite(local_y)||
       !isfinite(p->nocon_speed))return 0;
    if(world_y<=0){
        uint16_t next=17;
        if(action==9){if(animation==39)next=40;else if(animation==41)next=42;}
        *out=(SA3DSSpringActionResult){12,next,SA3DS_SPRING_MOVE_FALL};
    }else{
        *out=(SA3DSSpringActionResult){action,animation,
            action==10&&local_y>=p->nocon_speed?SA3DS_SPRING_MOVE_UNCONTROLLED:
                SA3DS_SPRING_MOVE_AIR_CONTROL};
    }
    return 1;
}
static float noinput_friction(float speed,float acceleration,float normal,
    float axis_friction,float multiplier,float limit_friction){
    if(speed==0){
        float limit=limit_friction*multiplier*normal;
        return acceleration<limit&&-limit<acceleration?0:acceleration;
    }
    float friction=normal<0?axis_friction*multiplier*normal:0;
    if(acceleration>0)friction=-friction;
    else if(acceleration==0)friction=0;
    acceleration+=friction;
    if((speed>0&&speed < -acceleration)||(speed<0&&-acceleration<speed))return -speed;
    return acceleration;
}
int sa3ds_spring_uncontrolled_add_velocity(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector force,const SA1PC2004MotionContext *ctx,
    const SA1SteamPlayerParameters *p,SA1PC2004LocalVector *out){
    if(!out)return 0;
    *out=(SA1PC2004LocalVector){0};
    if(!ctx||!p||ctx->status_0x4000||!isfinite(ctx->acceleration_multiplier)||
       !isfinite(speed.forward)||!isfinite(speed.normal)||!isfinite(speed.side)||
       !isfinite(force.forward)||!isfinite(force.normal)||!isfinite(force.side))return 0;
    SA1PC2004LocalVector add={force.forward+p->air_resist*speed.forward,
        force.normal+p->air_resist_y*speed.normal,
        force.side+p->air_resist_z*speed.side};
    if(ctx->grounded_status){
        add.forward=noinput_friction(speed.forward,add.forward,add.normal,
            p->grd_frict,ctx->acceleration_multiplier,p->lim_frict);
        add.side=noinput_friction(speed.side,add.side,add.normal,
            p->grd_frict_z,ctx->acceleration_multiplier,p->lim_frict);
    }
    *out=add;return 1;
}
int sa3ds_spring_control_blocked(int16_t timer){return timer!=0;}
int16_t sa3ds_spring_control_timer_tail(int16_t timer){return timer>0?(int16_t)(timer-1):0;}
SA1Input sa3ds_spring_filter_analog(SA1Input in,int16_t timer){
    if(sa3ds_spring_control_blocked(timer))in.move_x=in.move_z=0;
    return in;
}
int sa3ds_springb_primary_overlap(const SA3DSMovementPlacement *p,
    SA1Vec3 center,float radius){
    if(!p||(p->type&4095)!=2||!finite3(p->position)||!finite3(center)||
       !isfinite(radius)||radius<0)return 0;
    /* 41AA39..61 retains coordinate differences/products before one f32
     * distance store; radius sum remains retained for the squared compare.
     * Double approximates those retained values, not a complete x87 model. */
    double dx=(double)center.x-p->position.x,dy=(double)center.y-p->position.y,
        dz=(double)center.z-p->position.z;
    volatile double xx=dx*dx,yy=dy*dy,zz=dz*dz;
    float distance=(float)((xx+yy)+zz);
    double sum=(double)radius+14.0;
    return (double)distance<sum*sum;
}
int sa3ds_spring_primary_overlap(const SA3DSMovementPlacement *p,
    const SA3DSSpringPrimaryCollider *c){
    if(!p||!c||!finite3(p->position)||!finite3(c->task_origin)||!finite3(c->center)||
       !isfinite(c->radius)||!isfinite(c->half_height)||!isfinite(c->foot_radius)||
       c->radius<0||c->half_height<0||c->foot_radius<0)return -1;
    unsigned kind=p->type&4095;
    if(kind==2)return sa3ds_springb_primary_overlap(p,c->center,c->radius);
    if(kind!=1||c->gravity_axis!=3)return -1;
    /* 41C5D0 world-Y mode constructs task+(0,b,0), independent of the
     * separately absolute primary sphere center used by SpringB. */
    float py=c->task_origin.y+c->half_height;
    float top=py+c->half_height,bottom=py-c->half_height;
    float spring_top=p->position.y+10.f,spring_bottom=p->position.y-10.f;
    double dx=(double)c->task_origin.x-p->position.x;
    double dz=(double)c->task_origin.z-p->position.z;
    volatile double xx=dx*dx,zz=dz*dz;
    float distance=(float)(xx+zz);
    /* Retain the original asymmetric top-contact inset threshold. */
    double split=(double)top*0.4f+(double)bottom*0.6f;
    double inset=(double)c->foot_radius+6.f;
    if(bottom<spring_top&&spring_top<split&&(double)distance<inset*inset)return 1;
    double radius=(double)c->radius+6.f;
    if((double)distance>=radius*radius)return 0;
    if(top<=spring_bottom||spring_bottom+2.f<=top)
        return bottom<spring_top&&spring_bottom<top&&split<spring_top;
    return 1;
}
