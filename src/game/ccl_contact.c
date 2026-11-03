#include "ccl_contact.h"
#include <math.h>
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static float distance(SA1Vec3 a,SA1Vec3 b){double x=(double)a.x-b.x,y=(double)a.y-b.y,z=(double)a.z-b.z;return (float)(x*x+y*y+z*z);}
uint8_t sa3ds_sonic_ccl_damage_byte(uint8_t previous,uint8_t action,uint16_t status,uint16_t flags4,uint16_t flags6){
    unsigned attack=0,defense=0;
    if((flags6|flags4)&0x8000)attack=defense=3;
    else if(action==44){attack=2;defense=3;}
    else if(action==6){attack=1;defense=3;}
    else if(status&0x100)attack=defense=1;
    else if(action==45)defense=1;
    return (uint8_t)((previous&0xf0u)|attack|(defense<<2));
}
uint32_t sa3ds_sonic_ccl_secondary_attributes(uint32_t previous,uint8_t action){return action==72?previous&~0x10u:previous|0x10u;}
int sa3ds_ccl_shape_decode(SA3DSCCLShape*out,const void*record,SA1Vec3 center){
    if(!out)return 0;
    memset(out,0,sizeof(*out));if(!record||!finite3(center))return 0;
    const uint8_t*p=record;float radius=rf(p+20);
    if(!isfinite(radius)||radius<0)return 0;
    *out=(SA3DSCCLShape){p[1],p[2],p[3],r32(p+4),center,radius};return 1;
}
void sa3ds_ccl_body_init(SA3DSCCLBody*b,uint32_t id){
    if(!b)return;
    memset(b,0,sizeof(*b));sa3ds_ccl_hits_reset(&b->hits,id);b->cached_self_shape=b->cached_other_shape=-1;
}
void sa3ds_ccl_body_clear(SA3DSCCLBody*b,SA3DSCCLShape*shapes,unsigned count){
    if(!b||(!shapes&&count))return;
    b->outer_flags&=0xffc0u;b->status&=(uint16_t)~0x3cu;b->hit_count=0;
    b->cached_self_shape=b->cached_other_shape=-1;b->cached_other_id=0;
    for(unsigned i=0;i<count;i++)shapes[i].damage&=(uint8_t)~0x10u;
    for(unsigned i=0;i<16;i++){
        b->hits.hits[i]=(SA3DSCCLHit){-1,-1,0};b->hit_flags[i]&=0xfffcu;
    }
}
static void append(SA3DSCCLBody*self,const SA3DSCCLShape*shape,unsigned own,SA3DSCCLBody*other,const SA3DSCCLShape*other_shape,unsigned theirs){
    if((shape->attributes&8)||(other_shape->attributes&4))return;
    self->outer_flags|=1;self->status|=8;
    for(unsigned i=0;i<16;i++)if(self->hits.hits[i].other_shape==-1){
        self->hits.hits[i]=(SA3DSCCLHit){(int8_t)own,(int8_t)theirs,other->hits.actor_id};self->hit_count++;break;
    }
    /*Source updates this cache even if all16 list slots were occupied. */
    self->cached_self_shape=(int16_t)own;self->cached_other_shape=(int16_t)theirs;self->cached_other_id=other->hits.actor_id;
}
static void damage(SA3DSCCLBody*victim,SA3DSCCLShape*self,const SA3DSCCLShape*other){
    if((self->attributes&0x400)&&((self->damage>>2)&3)<(other->damage&3)&&
       (victim->rules.damage_receive&other->damage)){
        victim->outer_flags|=0x20;self->damage|=0x10;victim->status|=4;
    }
}
int sa3ds_ccl_record_contact(SA3DSCCLBody*a,SA3DSCCLShape*as,unsigned ai,SA3DSCCLBody*b,SA3DSCCLShape*bs,unsigned bi){
    if(!a||!b||a==b||!as||!bs||!a->hits.actor_id||!b->hits.actor_id||a->hits.actor_id==b->hits.actor_id||!a->hits.has_ccl||!b->hits.has_ccl||ai>127||bi>127)return 0;
    append(a,as,ai,b,bs,bi);append(b,bs,bi,a,as,ai);
    damage(b,bs,as);damage(a,as,bs); /*418DE0 first-record argument order*/
    return 1;
}
static int push_enabled(const SA3DSCCLBody*self,const SA3DSCCLShape*own,const SA3DSCCLBody*other,const SA3DSCCLShape*theirs){
    return (self->rules.push_receive&theirs->push)&&
        !((own->attributes&0x4000)&&(theirs->attributes&0x4000)&&(theirs->damage&0x10))&&
        !(other->rules.push_inhibit&own->push);
}
static void sphere_response(SA3DSCCLBody*a,SA3DSCCLShape*as,SA3DSCCLBody*b,SA3DSCCLShape*bs,float squared,SA3DSCCLContactResult*r){
    float dx=(float)((double)as->center.x-bs->center.x),dz=(float)((double)as->center.z-bs->center.z);
    double square=(double)dx*dx+(double)dz*dz;
    /*41AA00 uses787CC0's f32 squared store for the zero test, then
     *418AC0 normalizes with its retained sum. Vertical difference is zero. */
    if((float)square==0){dx=0;dz=.1f;}
    else{double inverse=1./sqrt(square);dx=(float)(dx*inverse);dz=(float)(dz*inverse);}
    a->direction_x=dx;a->direction_z=dz;
    float sum=(float)((double)as->radius+bs->radius),depth=(float)((double)sum-sqrt((double)squared));
    double x=(double)depth*dx;float z=(float)((double)depth*dz);
    if(push_enabled(a,as,b,bs)){
        a->outer_flags|=0x10;a->task_position.x=(float)((double)a->task_position.x+x);a->task_position.z=(float)((double)a->task_position.z+z);r->pushed_first=1;
    }
    if(push_enabled(b,bs,a,as)){
        b->outer_flags|=0x10;b->task_position.x=(float)((double)b->task_position.x-x);b->task_position.z=(float)((double)b->task_position.z-z);r->pushed_second=1;
    }
}
int sa3ds_ccl_sphere_contact(SA3DSCCLBody*a,SA3DSCCLShape*as,unsigned ai,SA3DSCCLBody*b,SA3DSCCLShape*bs,unsigned bi,SA3DSCCLContactResult*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!a||!b||a==b||!as||!bs||!a->hits.actor_id||!b->hits.actor_id||a->hits.actor_id==b->hits.actor_id||!a->hits.has_ccl||!b->hits.has_ccl||ai>127||bi>127||!finite3(a->task_position)||!finite3(b->task_position)||!finite3(as->center)||!finite3(bs->center)||
       !isfinite(a->broad_radius)||a->broad_radius<0||!isfinite(b->broad_radius)||b->broad_radius<0||
       !isfinite(as->radius)||as->radius<0||!isfinite(bs->radius)||bs->radius<0)return 0;
    SA3DSCCLContactResult r={.task_distance_squared=distance(a->task_position,b->task_position)};
    double sum=(double)a->broad_radius+b->broad_radius;r.broad_overlap=r.task_distance_squared<sum*sum;
    if(!(as->attributes&0x10)&&!(bs->attributes&0x10)&&!(a->rules.excluded_attributes&bs->attributes)&&!(b->rules.excluded_attributes&as->attributes))r.eligible=1;
    r.supported=ai==0?(as->form==0||as->form==7)&&(bs->form==0||bs->form==7):(as->form==0&&bs->form==0);
    if(r.broad_overlap&&r.eligible&&r.supported){
        sum=(double)as->radius+bs->radius;
        float squared=distance(as->center,bs->center);
        if(squared<sum*sum){
            if(!sa3ds_ccl_record_contact(a,as,ai,b,bs,bi))return 0;
            r.overlap=1;sphere_response(a,as,b,bs,squared,&r);
        }
    }
    *out=r;return 1;
}
