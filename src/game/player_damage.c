#include "player_damage.h"
#include <math.h>
#include <string.h>
static int signed_byte(uint8_t u){int8_t n;memcpy(&n,&u,1);return n;}
static int finite3(SA1Vec3 v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
static void sound(SA3DSPlayerDamageResult*r,uint16_t id,uint8_t channel){
    if(r->sound_count<2){unsigned i=r->sound_count++;r->sound_ids[i]=id;r->sound_channels[i]=channel;}
}
static int ring_loss(SA3DSPlayerDamageState*s,const SA3DSPlayerDamageWorld*w,SA3DSPlayerDamageResult*r){
    if(!w->random15)return 0;
    uint16_t random=w->random15(w->context);if(random>32767)return 0;
    float seed=(float)((double)random*(double)(1.f/32768.f)*(double)360.f);
    if(s->player_index)return 1; /*source still consumes RNG/reads count*/
    int count=s->rings;
    if(count==0){
        if(w->death_command_allowed){s->status|=0x1000;s->next_command=50;s->flags6|=0x4000;r->death_requested=1;}
        sound(r,w->game_mode==6?0x597:0x17,0);return 1;
    }
    if(count>20)count=20;
    s->rings=0;r->reset_rings=1;r->scatter_origin=s->position;
    if(count>0){
        r->scatter_count=(uint8_t)count;
        for(int i=0;i<count;i++){
            /*Source integer quotient is recomputed for each task, not a
             * constant angular step (e.g.1080/7 is154, not3*51).*/
            float degrees=(float)(i*360/count)+seed;
            r->scatter_angles[i]=(int32_t)trunc((double)degrees*65536.*(1./360.));
        }
    }
    sound(r,0,0);return 1;
}
int sa3ds_player_damage_tick(SA3DSPlayerDamageState*state,const SA3DSPlayerDamageWorld*w,const SA1SteamPlayerParameters*p,SA3DSPlayerDamageResult*out){
    if(!state||!w||!p||!out||!finite3(state->position)||!finite3(state->collision_up)||
       !isfinite(state->speed.forward)||!isfinite(state->speed.normal)||!isfinite(state->speed.side))return 0;
    SA3DSPlayerDamageState s=*state;SA3DSPlayerDamageResult r={0};
    if(s.flags6&0x8000){s.status|=0x8000;s.timer=0;goto done;}
    if(s.timer){s.status&=(uint16_t)~4u;s.timer=(uint16_t)(s.timer-1);goto done;}
    r.checked=1;
    if(s.task_byte2==2||(!(s.status&4)&&!(s.flags5c&0x10000))){s.status&=(uint16_t)~0x8000u;goto done;}
    s.timer=120;r.rumble=1;int action=signed_byte(s.action);
    if(action>=46&&action<=50){
        if(!w->alternate_character)sound(&r,0x4c6,0);
        s.action=50;s.animation=46;r.water_effect=1;r.accepted=1;
        if(s.rings>0){s.position.x=(float)((double)s.position.x+s.collision_up.x*5.);
            s.position.y=(float)((double)s.position.y+s.collision_up.y*5.);
            s.position.z=(float)((double)s.position.z+s.collision_up.z*5.);
            s.speed.normal=p->jmp_y_spd;s.speed.forward*=.2f;s.angles[0]=0;s.angles[2]=0xc000;}
    }else if(action>=62&&action<=68){
        sound(&r,w->alternate_character?0x7fd:0x4d1,w->alternate_character?1:0);
        s.action=68;s.animation=110;s.status&=(uint16_t)~4u;
    }else if(s.flags4&0x8000){s.speed.forward*=.5f;
    }else if(action!=45){
        if(action==44&&(w->stage_id<15||w->stage_id>25))r.release_light_dash=1;
        sound(&r,w->alternate_character?0x7fd:0x4d1,w->alternate_character?1:0);
        s.action=16;s.animation=23;s.status&=(uint16_t)~4u;
        r.shader_changed=1;r.shader_index=8;s.speed.forward=-p->run_speed;s.speed.normal=p->jmp_y_spd;
        s.status&=(uint16_t)~0x500u;r.accepted=1;
    }
    if(!r.accepted){s.status&=(uint16_t)~0x8000u;goto done;}
    s.status|=0x8000;if(s.status&0x800)r.release_held=1;
    s.status&=0xc4ffu;
    if(s.flags6&3){s.flags6&=(uint16_t)~3u;r.shield_consumed=1;}
    else if(!ring_loss(&s,w,&r))return 0;
done:*state=s;*out=r;return 1;
}
int sa3ds_player_hurt_select(SA3DSPlayerDamageState*s,int analog,int jump,SA3DSPlayerHurtResult*out){
    if(!s||!out)return 0;
    *out=(SA3DSPlayerHurtResult){0};if(s->action!=16)return 1;
    if(s->status&(0x1000|0x800))return 0; /*commands/carried-object handler owned by caller*/
    if(s->animation!=23&&s->animation!=24){
        s->action=(!analog&&s->speed.forward==0.f)?1:2;
        out->resume=1;out->idle_selector=s->action==1;out->ground_selector=s->action==2;return 1;
    }
    if(s->timer){
        if(s->timer>60||!analog){
            if(s->timer!=120&&(s->status&3)){s->animation=24;out->write_animation=1;}
            return 1;
        }
        if((s->status&3)&&jump){out->begin_jump=1;out->resume=1;return 1;}
        if(!(s->status&3))return 1;
        s->action=2;out->resume=out->ground_selector=1;return 1;
    }
    if(!(s->status&3)){s->action=12;s->animation=18;out->resume=out->write_animation=1;return 1;}
    s->action=2;out->resume=out->ground_selector=1;return 1;
}
int sa3ds_player_damage_visible(uint16_t timer){return (timer&2u)==0;}
