#include "saru_actor.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static int16_t s16(uint16_t n){int16_t v;memcpy(&v,&n,2);return v;}
static int32_t s32(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static int32_t add_angle(int32_t a,int32_t b){return s32((uint32_t)a+(uint32_t)b);}
static int32_t approach(int32_t a,int32_t b){
    int d=s16((uint16_t)((uint16_t)b-(uint16_t)a));
    return d>0x400?(uint16_t)(a+0x400):d< -0x400?(uint16_t)(a-0x400):(uint16_t)b;
}
static unsigned distance_angle(int32_t a,int32_t b){int d=s16((uint16_t)((uint16_t)b-(uint16_t)a));return (unsigned)(d<0?-d:d);}
static double distance(SA1Vec3 a,SA1Vec3 b){double x=(double)a.x-b.x,y=(double)a.y-b.y,z=(double)a.z-b.z;return x*x+y*y+z*z;}
static double random_unit(const SA3DSSaruServices*v){return v->random15(v->context)/32768.;}
static void request(SA3DSSaruActor*a,unsigned action,unsigned clip,float rate,float frame){
    a->previous_action=a->action;a->action=(uint8_t)action;a->counter=0;
    sa3ds_saru_animation_request(&a->animation,(uint16_t)clip,frame,rate);
}
static void sound(SA3DSSaruActorResult*r,unsigned id){if(r->sound_count<2){unsigned i=r->sound_count++;r->sound_ids[i]=(uint16_t)id;r->sound_positional[i]=(id==0x21);}}
static void kill(SA3DSSaruActor*a){a->action=12;a->death_byte9=0;a->death_byte10=1;}
static void orbit(SA3DSSaruActor*a,const SA3DSSaruServices*v){
    uint16_t phase=(uint16_t)a->angles[0];
    float sx=(float)v->sine(v->context,phase),cx=(float)v->sine(v->context,(uint16_t)(phase+0x4000));
    float x=(float)((double)cx*a->scale.z),z=(float)(-(double)sx*a->scale.z);
    a->position.x=(float)((double)a->origin.x+x);a->position.z=(float)((double)a->origin.z+z);
}
static void choose_orbit(SA3DSSaruActor*a,const SA3DSSaruActorInput*in,const SA3DSSaruServices*v){
    /*4ABA80 subtracts into a float vector before testing origin distance. */
    float dx=(float)((double)in->player_position.x-a->origin.x),dy=(float)((double)in->player_position.y-a->origin.y),dz=(float)((double)in->player_position.z-a->origin.z);
    double d=(double)dx*dx+(double)dy*dy+(double)dz*dz;
    double outer=(double)a->scale.z+140.f,inner=(double)a->scale.z+20.f;
    if(d>outer*outer){a->target_orbit=(int32_t)trunc(random_unit(v)*360.*65536.*(1./360.));return;}
    int32_t angle=v->atan_bams(v->context,(double)in->player_position.x-a->origin.x,(double)in->player_position.z-a->origin.z);
    a->target_orbit=add_angle(angle,d>inner*inner?-0x4000:0x4000);
}
static int arbitrate(SA3DSSaruActor*a,const SA3DSSaruActorInput*in,const SA3DSSaruServices*v){
    double d=distance(in->player_position,a->position);
    if(d<625.){
        choose_orbit(a,in,v);unsigned action=random_unit(v)>.5?3:5;
        request(a,action,0,.5f,0);return 1;
    }
    if(d<40000.&&a->counter>(uint16_t)a->angles[2])request(a,7,0,.1f,0);
    if((a->status&0xc00)==0xc00){request(a,7,0,.1f,0);a->status&=(uint16_t)~0x800u;return 1;}
    return 0; /*The middle-distance transition does not return true. */
}
static void squash(SA3DSSaruActor*a){
    double next=(double)a->squash_velocity-((double)a->squash_velocity*.2f+((double)a->scale.y-1.)*.32f);
    a->squash_velocity=(float)next;
    next+=a->scale.y;a->scale.y=(float)next;
    if(next>1.7f)a->scale.y=1.7f;else if(next<.3f)a->scale.y=.3f;
    a->scale.x=(float)(2.-a->scale.y);
}
static void draw(SA3DSSaruActor*a,const SA3DSSaruActorInput*in,const SA3DSSaruServices*v,SA3DSSaruActorResult*r,unsigned variant){
    if(variant==0&&!in->visible)return;
    /*Scale arguments are captured before squash; source fields are f32. */
    float x=(float)((double)a->scale.x*1.2f),y=(float)((double)a->scale.y*1.2f);
    if(variant!=2&&(a->status&0x2000))a->position.y=(float)(v->floor_height(v->context,a)+4.);
    squash(a);
    if(in->render_suppressed)return;
    r->draw=1;r->logic_draw=(SA3DSSaruDraw){.position=a->position,.scale={x,y,x},.bob=a->bob,.variant=(uint8_t)variant};
    memcpy(r->logic_draw.angles,a->angles,sizeof(a->angles));
    int blending=(a->animation.enemy_flags&0x200)!=0;
    sa3ds_saru_animation_logic_draw(&a->animation,1,&r->logic_draw.animation);
    if(variant==2&&!blending&&(a->status&0x200)){
        double next=(double)a->bomb_scale+.1f;a->bomb_scale=(float)next;if(next>1.)a->bomb_scale=1;
        if(a->animation.current_frame>9.5f){
            a->status&=(uint16_t)~0x200u;a->bomb_scale=0;r->projectile=1;r->projectile_pose=r->logic_draw;
        }
    }
}
int sa3ds_saru_actor_init(SA3DSSaruActor*a,const SA3DSMovementPlacement*p,const SA3DSSaruServices*v){
    if(!a)return 0;
    memset(a,0,sizeof(*a));if(!p||!v||!v->sine||!v->random15)return 0;
    SA3DSSaruInitial initial;uint16_t phase=(uint16_t)p->angles[0];
    uint16_t random=v->random15(v->context);
    double sine=v->sine(v->context,phase),cosine=v->sine(v->context,(uint16_t)(phase+0x4000));
    if(!sa3ds_saru_initialize(p,random,sine,cosine,&initial))return 0;
    a->origin=initial.spawn_origin;a->position=initial.task_position;a->scale=initial.task_scale;
    memcpy(a->angles,initial.task_angles,sizeof(a->angles));a->player_yaw=initial.initial_angle_y;
    a->status=initial.task_status;a->packed_tag=initial.packed_tag_low;a->mode=initial.mode;a->wait=initial.index_seed;
    sa3ds_saru_animation_init(&a->animation);return 1;
}
int sa3ds_saru_actor_tick(SA3DSSaruActor*a,const SA3DSSaruActorInput*in,const SA3DSSaruServices*v,SA3DSSaruActorResult*r){
    if(!r)return 0;
    memset(r,0,sizeof(*r));
    if(!a||!in||!v||!v->sine||!v->atan_bams||!v->random15||!v->damage||
       ((a->status&0x2000)&&!v->floor_height)||!in->action_frames||!finite3(in->player_position)||!finite3(a->position)||
       !finite3(a->origin)||!finite3(a->scale)||a->scale.z<1||a->animation.current_action>=6||a->animation.pending_action>=6)return 0;
    for(unsigned i=0;i<6;i++)if(!in->action_frames[i])return 0;
    unsigned state=a->action;float frames;double next;
    switch(state){
    case 0:{
        float probability=(float)((double)((uint32_t)a->angles[2]&255u)*.003f);
        if(in->collision_flag0&&v->damage(v->context,a))kill(a);
        float choice=(float)random_unit(v);draw(a,in,v,r,0);
        if(arbitrate(a,in,v))break;
        if(!(a->animation.enemy_flags&0x200)){
            a->animation.current_frame=(float)(v->sine(v->context,(uint16_t)a->idle_phase)*.5f+3.);a->idle_phase+=0x600;
        }
        if(a->counter<=a->wait)break;
        a->idle_phase=0;
        if(choice<probability){
            if(a->status&0x400){request(a,10,0,.085f,0);sound(r,0x361);a->animation.pending_action=random_unit(v)>.5?1:2;}
            else if(random_unit(v)>.5)request(a,1,0,.3f,0);else request(a,2,5,.085f,0);
        }else{
            choose_orbit(a,in,v);a->previous_action=a->action;a->action=choice<(probability+1.)*.5?5:3;a->counter=0;
        }
        break;}
    case 1:case 2:
        draw(a,in,v,r,0);if(state==1&&arbitrate(a,in,v))break;
        if(a->animation.enemy_flags&0x200)break;
        frames=in->action_frames[a->animation.current_action];next=(double)a->animation.current_frame+(state==1?.45f:.28f);a->animation.current_frame=(float)next;
        if(next>=frames){
            a->animation.current_frame=(float)((double)frames-.1f);if(state==1)r->dust_bursts++;
            a->wait=(uint8_t)(int32_t)trunc(random_unit(v)*(state==1?30.:40.)+10.);request(a,0,0,state==1?.1f:.085f,3);
        }break;
    case 3:case 5:
        draw(a,in,v,r,0);if(a->animation.enemy_flags&0x200)break;
        a->animation.current_frame=(float)((double)a->animation.current_frame+.45f);
        a->angles[1]=approach(a->angles[1],state==3?0x4000:0xc000);
        frames=in->action_frames[a->animation.current_action];
        if(a->animation.current_frame>=frames){a->animation.current_frame=(float)((double)frames-.1f);r->dust_bursts++;
            a->wait=(uint8_t)(int32_t)trunc(random_unit(v)*30.+10.);request(a,state==3?4:6,3,.3f,0);}
        break;
    case 4:case 6:
        draw(a,in,v,r,1);if(a->animation.enemy_flags&0x200)break;
        a->animation.current_frame=(float)((double)a->animation.current_frame+(state==4?.8f:1.2f));
        next=(1./((double)a->scale.z+3.))*65536.*(state==4?.15915497620314795:-.15915497620314795);
        a->angles[0]=add_angle(a->angles[0],(int32_t)trunc(next));orbit(a,v);
        a->bob=(float)(fabs(v->sine(v->context,(uint16_t)a->hop_phase))*(state==4?6.:7.));a->hop_phase+=state==4?0x670:0x9a8;
        frames=in->action_frames[a->animation.current_action];
        if(a->animation.current_frame<frames)break;
        a->hop_phase=0;a->animation.current_frame=(float)((double)a->animation.current_frame-frames);r->dust_bursts++;
        if(state==6)sound(r,0x21);
        if(arbitrate(a,in,v))break;
        if(a->counter>120&&distance(in->player_position,a->position)<625.){choose_orbit(a,in,v);request(a,state==4?5:3,0,.4f,0);}
        else if(a->counter>(uint16_t)a->angles[2]||distance_angle(a->target_orbit,a->angles[0])<0x1000){
            a->wait=(uint8_t)(int32_t)trunc(random_unit(v)*20.+10.);request(a,7,0,.1f,0);
        }break;
    case 7:
        a->player_yaw=add_angle(v->atan_bams(v->context,(double)in->player_position.x-a->position.x,(double)in->player_position.z-a->position.z),-0x4000);
        if(!(a->animation.enemy_flags&0x200)){
            a->animation.current_frame=(float)((double)a->animation.current_frame+.45f);
            a->angles[1]=approach(a->angles[1],s32((uint32_t)a->player_yaw-(uint32_t)a->angles[0]));
            frames=in->action_frames[a->animation.current_action];
            if(a->animation.current_frame>=frames){
                a->animation.current_frame=(float)((double)frames-.1f);r->dust_bursts++;
                if(a->status&0x400)request(a,10,random_unit(v)>.5?1:2,.085f,0);
                else if(distance(in->player_position,a->position)<22500.&&fabs((double)in->player_position.y-a->position.y)<60.&&a->mode!=2){request(a,9,4,.085f,0);a->status|=0x200;}
                else if(random_unit(v)>.5)request(a,1,0,.3f,0);else request(a,2,5,.085f,0);
            }
        }
        draw(a,in,v,r,0);break;
    case 9:
        a->position=a->saved_position;draw(a,in,v,r,2);if(a->animation.enemy_flags&0x200)break;
        frames=in->action_frames[a->animation.current_action];next=(double)a->animation.current_frame+.3f;a->animation.current_frame=(float)next;
        if(next>=frames){a->animation.current_frame=frames-2.f;a->wait=(uint8_t)a->angles[2];request(a,0,0,.07f,3);sound(r,0x362);}break;
    case 10:
        draw(a,in,v,r,0);if(a->animation.enemy_flags&0x200)break;
        frames=in->action_frames[a->animation.current_action];next=(double)a->animation.current_frame+.4f;a->animation.current_frame=(float)next;
        if(next>=frames){r->dust_bursts++;
            if(random_unit(v)<.5){a->animation.current_frame=(float)((double)frames-.1f);a->wait=(uint8_t)(int32_t)trunc((random_unit(v)+1.)*10.);request(a,0,0,.1f,3);}
            else{sound(r,0x361);a->animation.current_frame=(float)((double)a->animation.current_frame-frames);}
        }break;
    case 12:r->death=1;r->destroy=1;return 1;
    default:break; /*8/11 and signed-byte out-of-range dispatch use common tail.*/
    }
    if(in->collision_flag0&&v->damage(v->context,a))kill(a);
    if(v->damage(v->context,a))kill(a);
    a->saved_position=a->position;
    a->counter=(uint16_t)(a->counter+1u);
    if(s32(a->age)>600)a->status&=(uint16_t)~0x400u;
    a->age++;r->enqueue_collision=(a->status&0x100)!=0;r->display=1;return 1;
}

int sa3ds_saru_damage_check(SA3DSSaruActor*a,const SA3DSEnemyDamageInput*world,SA3DSCCLQueryCursor*c,SA3DSEnemyDamageResult*out){
    if(!a||!world||!out)return 0;
    SA3DSEnemyDamageInput input=*world;input.task_status=a->status;input.position=a->position;
    SA3DSEnemyDamageState state={a->animation.enemy_flags,a->squash_velocity,a->damage_velocity_y};
    if(!sa3ds_enemy_damage_check(&state,&input,c,out))return 0;
    a->animation.enemy_flags=state.enemy_flags;a->squash_velocity=state.squash_velocity;a->damage_velocity_y=state.velocity_y;
    return 1;
}
