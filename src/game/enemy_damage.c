#include "enemy_damage.h"
#include <math.h>
#include <string.h>
void sa3ds_ccl_query_reset(SA3DSCCLQueryCursor*c){if(c)*c=(SA3DSCCLQueryCursor){.index=-1};}
void sa3ds_ccl_hits_reset(SA3DSCCLHits*h,uint32_t id){
    if(!h)return;
    memset(h,0,sizeof(*h));h->actor_id=id;h->has_ccl=1;
    for(unsigned i=0;i<SA3DS_CCL_HIT_CAPACITY;i++)h->hits[i].other_shape=-1;
}
int sa3ds_ccl_query_category(SA3DSCCLQueryCursor*c,const SA3DSCCLHits*h,unsigned category,SA3DSCCLResolveCategory resolve,void*context){
    if(!c||!h||category>1||!resolve||c->index< -1||c->index>16)return -2;
    uint32_t query=category?0x41c750u:0x41c6e0u;
    if(c->actor_id!=h->actor_id||c->query_id!=query){c->actor_id=h->actor_id;c->query_id=query;c->index=-1;}
    if(c->index<16)c->index++;
    if(!h->actor_id||!h->has_ccl)return -1;
    for(int i=c->index;i<16;i++){
        const SA3DSCCLHit*hit=&h->hits[i];unsigned current;
        if(hit->other_shape==-1)break;
        if(hit->actor_id&&resolve(context,hit->actor_id,&current)&&current==category){c->index=i;return i;}
    }
    return -1;
}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static int explosion(const SA3DSEnemyDamageInput*in){
    if(!(in->explosion_radius>=0))return 0;
    float x=(float)((double)in->position.x-in->explosion_center.x),y=(float)((double)in->position.y-in->explosion_center.y),z=(float)((double)in->position.z-in->explosion_center.z);
    /*787CC0 stores summed squared magnitude as f32 before7871F0 sqrt. */
    float squared=(float)((double)z*z+(double)y*y+(double)x*x);
    return sqrt((double)squared)<in->explosion_radius;
}
int sa3ds_enemy_damage_check(SA3DSEnemyDamageState*s,const SA3DSEnemyDamageInput*in,SA3DSCCLQueryCursor*c,SA3DSEnemyDamageResult*out){
    if(!out)return 0;
    *out=(SA3DSEnemyDamageResult){.bounce_player=-1};
    if(!s||!in)return 0;
    if(!(in->task_status&4)){
        if(in->explosion_radius>=0&&(!isfinite(in->explosion_radius)||!finite3(in->position)||!finite3(in->explosion_center)))return 0;
        out->killed=(uint8_t)explosion(in);return 1;
    }
    if(!c||!in->hits||!in->hits->has_ccl||!in->resolve)return 0;
    SA3DSCCLQueryCursor next_cursor=*c;
    int hit=sa3ds_ccl_query_category(&next_cursor,in->hits,0,in->resolve,in->resolve_context);
    if(hit==-1)hit=sa3ds_ccl_query_category(&next_cursor,in->hits,1,in->resolve,in->resolve_context);
    if(hit< -1||(hit>=0&&!in->cached_damage_other_valid))return 0;
    SA3DSEnemyDamageState next=*s;next.squash_velocity=.35f;
    SA3DSEnemyDamageResult r={.killed=1,.bounce_player=-1};
    if(hit>=0){
        next.enemy_flags|=0x1000;r.score_delta=10;
        if(in->cached_other_shape_attributes&0x1000){next.enemy_flags|=0x800;next.velocity_y=.7f;}
        else{
            unsigned player=in->cached_owner_is_player0?0:1;const SA3DSEnemyDamagePlayer*p=&in->players[player];
            if(p->present&&(p->status&0x100)&&!(p->status&3)&&(p->character==0||p->character==2||p->character==3)){
                r.bounce_player=(int)player;r.bounce_world_velocity=(SA1Vec3){0,2,0};r.rumble_request=1;
            }
            next.enemy_flags&=(uint16_t)~0x400u;
        }
    }
    *s=next;*c=next_cursor;*out=r;return 1;
}

int sa3ds_enemy_damage_apply_bounce(const SA3DSEnemyDamageResult*r,unsigned index,SA1PlayerState*p){
    if(!r||!p||index>1||r->bounce_player!=(int)index||!finite3(r->bounce_world_velocity))return 0;
    p->vel=r->bounce_world_velocity;return 1;
}
