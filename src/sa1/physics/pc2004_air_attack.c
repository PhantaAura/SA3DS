#include "pc2004_air_attack.h"
#include <string.h>
static int16_t increment(int16_t value){uint16_t bits=(uint16_t)((uint16_t)value+1u);int16_t result;memcpy(&result,&bits,2);return result;}
SA1PC2004AirAttackSelection sa1_pc2004_air_attack_select(
    SA1PC2004AirAttack current,int jump_pressed,int action_pressed,int category){
    SA1PC2004AirAttackSelection r={.next=current};
    if(current.action==8){
        if(action_pressed){
            r.next.action=12;r.next.animation=18;r.next.status&=(uint16_t)~0x500u;
            r.skip_jump_hold=1;
        }else if(jump_pressed&&!(current.status&0x4000u)&&!(current.status&3u)){
            /*43BF40 entry does not select a target;494B80 owns initialization. */
            r.next.action=14;r.next.timer=-1;r.next.age=0;r.entered=r.skip_jump_hold=1;
        }
    }else if(current.action==14){
        r.next.age=increment(current.age);r.skip_jump_hold=1;
        if(current.age>359||category==0||category==2||category==3){
            r.next.timer=-1;r.next.action=8;
        }else if(current.timer>=6||action_pressed){
            r.next.timer=0;r.next.action=12;r.next.animation=18;r.next.status&=(uint16_t)~0x500u;
        }
    }
    return r;
}
SA1PC2004AirAttackSelection sa1_pc2004_air_attack_select_no_target(
    SA1PC2004AirAttack current,int jump_pressed,int action_pressed,int category,int special_player){
    SA1PC2004AirAttackSelection r=sa1_pc2004_air_attack_select(current,jump_pressed,action_pressed,category);
    if(r.entered){r.next.timer=1;r.write_forward=1;r.forward=special_player?10.f:5.f;}
    return r;
}
int sa1_pc2004_air_attack_decay(SA1PC2004AirAttack *s,SA1PC2004LocalVector *speed){
    if(!s||!speed||s->action!=14||s->timer<=0)return 0;
    s->timer=increment(s->timer);speed->forward*=.9f;return 1;
}
