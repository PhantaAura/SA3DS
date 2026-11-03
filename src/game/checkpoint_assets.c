#include "checkpoint_assets.h"
#include "sa3dspak.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa3ds_checkpoint_assets_open(SA3DSCheckpointAssets*out,const void*bytes,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));if(!bytes||size>SA3DS_CHECKPOINT_MAX_BYTES)return 0;
    SA3DSPak pak;SA3DSPakChunk config;
    if(sa3dspak_open(&pak,bytes,size)!=SA3DSPAK_OK||pak.toc_count!=7)return 0;
    unsigned seen=0;
    for(unsigned i=0;i<7;i++){
        SA3DSPakChunk c;if(sa3dspak_chunk_at(&pak,i,&c)!=SA3DSPAK_OK||c.flags)return 0;
        unsigned bit=!strcmp(c.type,"METADATA")?1:!strcmp(c.type,"CHECKPOINT")?2:0;
        for(unsigned j=0;j<5;j++){char key[16];snprintf(key,sizeof(key),"VISUAL%u",j);if(!strcmp(c.type,key))bit=4u<<j;}
        if(!bit||(seen&bit))return 0;
        seen|=bit;
    }
    if(seen!=127||sa3dspak_find(&pak,"CHECKPOINT",&config)!=SA3DSPAK_OK||config.size<64)return 0;
    const uint8_t*b=config.data;
    uint32_t n=r32(b+12),co=64+n*40,po=co+48,ro=po+96,no=ro+16,land=no+36,lo=land+24,total=lo+4100;
    if(memcmp(b,"SA3CHKP\0",8)||r32(b+8)!=2||!n||n>32||r32(b+16)!=64||
       r32(b+20)!=co||r32(b+24)!=po||r32(b+28)!=ro||r32(b+32)!=no||
       r32(b+36)!=lo||r32(b+40)!=total||total!=config.size||
       rf(b+44)!=360000.f||rf(b+48)!=360100.f||r32(b+52)!=land||r32(b+56)||r32(b+60))return 0;
    SA3DSCheckpointAssets next={0};next.placements=(SA3DSMovementObjects){b+64,NULL,n};
    next.collider=b+co;next.player_colliders=b+po;next.quarter_lut=b+lo;
    next.load_squared=rf(b+44);next.unload_squared=rf(b+48);
    SA3DSMovementPlacement previous={0};
    for(unsigned i=0;i<n;i++){
        SA3DSMovementPlacement p;if(!sa3ds_movement_objects_placement(&next.placements,i,&p)||p.act>1||p.source_row>=1023||
            (p.type&4095)!=19||!finite3(p.position))return 0;
        for(unsigned a=0;a<3;a++)if(!isfinite(p.parameters[a]))return 0;
        if(i&&(p.act<previous.act||(p.act==previous.act&&p.source_row<=previous.source_row)))return 0;
        previous=p;
    }
    for(unsigned i=0;i<3;i++){
        const uint8_t*p=i?next.player_colliders+(i-1)*48:next.collider;
        for(unsigned a=0;a<7;a++)if(!isfinite(rf(p+8+a*4))||fabsf(rf(p+8+a*4))>1024)return 0;
        if(rf(p+20)<=0||p[0]!=(i==2?2:0)||p[1]!=(i==0?3:i==1?7:1))return 0;
    }
    for(unsigned i=0;i<2;i++){
        const uint8_t*p=b+ro+i*8;if(p[3])return 0;
        next.rules[i]=(SA3DSCCLRules){p[0],p[1],p[2],r32(p+4)};
    }
    for(unsigned i=0;i<3;i++)for(unsigned a=0;a<3;a++){
        float f=rf(b+no+i*12+a*4);if(!isfinite(f)||fabsf(f)>256)return 0;next.node_positions[i][a]=f;
    }
    for(unsigned i=0;i<2;i++)for(unsigned a=0;a<3;a++){
        float f=rf(b+land+i*12+a*4);if(!isfinite(f)||fabsf(f)>256)return 0;next.land_offsets[i][a]=f;
    }
    for(unsigned i=0;i<=1024;i++){
        float f=rf(next.quarter_lut+i*4);if(!isfinite(f)||f<0||f>1||(i&&f<rf(next.quarter_lut+(i-1)*4)))return 0;
    }
    if(rf(next.quarter_lut)!=0||rf(next.quarter_lut+4096)!=1)return 0;
    for(unsigned i=0;i<5;i++){
        char key[16];snprintf(key,sizeof(key),"VISUAL%u",i);SA3DSPakChunk c;
        if(sa3dspak_find(&pak,key,&c)!=SA3DSPAK_OK||
            sa3ds_ec_visual_open(&next.components[i],c.data,(size_t)c.size)!=SA3DS_EC_VISUAL_OK)return 0;
        SA3DSECVisual*v=&next.components[i];
        if(v->model_count!=1||v->instance_count!=1||v->vertex_count>4096||v->index_count>8192||v->material_count>16||v->textures.texture_count>16)return 0;
        size_t texbytes=0;
        for(unsigned j=0;j<v->textures.texture_count;j++){
            SA3TTexture t;const uint8_t*p;
            if(sa3t_texture(&v->textures,j,&t,&p)!=SA3T_OK||t.width<8||t.height<8||t.width>256||t.height>256||
                (t.width&(t.width-1))||(t.height&(t.height-1)))return 0;
            texbytes+=t.data_size;
        }
        if(texbytes>256u*1024u)return 0;
    }
    *out=next;return 1;
}
static double sample(const uint8_t*t,unsigned i){
    i&=4095;
    if(i<1024)return rf(t+i*4);
    if(i<2048)return rf(t+(2048-i)*4);
    if(i<3072)return -rf(t+(i-2048)*4);
    return -rf(t+(4096-i)*4);
}
static double sine(const uint8_t*t,uint16_t angle){
    unsigned i=angle>>4,f=angle&15;double a=sample(t,i);
    return f?a+(sample(t,i+1)-a)*(f*.0625):a;
}
static void rotate(SA3CMatrix*m,const uint8_t*lut,unsigned axis,uint16_t angle){
    if(!angle)return;
    float s=(float)sine(lut,angle),c=(float)sine(lut,(uint16_t)(angle+16384));
    unsigned a=axis==0?1:0,b=axis==2?1:2;
    for(unsigned r=0;r<4;r++){
        float x=m->m[a*4+r],y=m->m[b*4+r];
        if(axis==1){m->m[a*4+r]=(float)((double)x*c-(double)y*s);m->m[b*4+r]=(float)((double)y*c+(double)x*s);}
        else{m->m[a*4+r]=(float)((double)x*c+(double)y*s);m->m[b*4+r]=(float)((double)y*c-(double)x*s);}
    }
}
static void translate(SA3CMatrix*m,const float p[3]){
    for(unsigned r=0;r<4;r++)m->m[12+r]=(float)((double)p[2]*m->m[8+r]+(double)p[1]*m->m[4+r]+(double)p[0]*m->m[r]+m->m[12+r]);
}
int sa3ds_checkpoint_pose(const SA3DSCheckpointAssets*a,const SA3DSMovementPlacement*p,
    const SA3DSCheckpointResult*draw,SA3CMatrix world[3]){
    if(!a||!a->quarter_lut||!p||!draw||!world||!finite3(p->position))return 0;
    sa3c_matrix_identity(&world[0]);world[0].m[12]=p->position.x;world[0].m[13]=p->position.y;world[0].m[14]=p->position.z;
    rotate(&world[0],a->quarter_lut,2,(uint16_t)p->angles[2]);
    rotate(&world[0],a->quarter_lut,0,(uint16_t)p->angles[0]);
    rotate(&world[0],a->quarter_lut,1,(uint16_t)p->angles[1]);
    for(unsigned i=1;i<3;i++){
        world[i]=world[0];translate(&world[i],a->node_positions[i]);
        rotate(&world[i],a->quarter_lut,1,(uint16_t)(i==1?draw->draw_post_y:-draw->draw_post_y));
        rotate(&world[i],a->quarter_lut,0,(uint16_t)draw->draw_post_x);
    }
    return 1;
}
