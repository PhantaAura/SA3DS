#include "sa3c_render.h"

#include <string.h>

#define SA3C_INVALID_U16 0xFFFFu
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static const uint8_t *sec(const SA3CCharacter *c,unsigned i){return c->data+c->sections[i].offset;}

static SA3CPoseResult emit_node(const SA3CCharacter *c,uint16_t ni,const SA3CMatrix *world,SA3CDrawVisitor visitor,void *user,SA3CRenderStats *st,uint32_t depth){
    if(ni==SA3C_INVALID_U16)return SA3C_POSE_OK;
    if((uint32_t)ni>=c->sections[0].count||depth>c->sections[0].count)return SA3C_POSE_ERR_HIERARCHY;
    const uint8_t *n=sec(c,0)+(size_t)ni*c->sections[0].stride;
    uint16_t model=rd16(n),child=rd16(n+2),sibling=rd16(n+4); uint32_t flags=rd32(n+8); st->visited_nodes++;
    if(model!=SA3C_INVALID_U16 && !(flags&SA3C_EVAL_HIDE)){
        if((uint32_t)model>=c->sections[1].count)return SA3C_POSE_ERR_RANGE;
        st->visible_model_nodes++;
        const uint8_t *m=sec(c,1)+(size_t)model*c->sections[1].stride;
        uint32_t first_mesh=rd32(m),mesh_count=rd32(m+4);
        for(uint32_t j=0;j<mesh_count;j++){
            uint32_t mesh_i=first_mesh+j; if(mesh_i>=c->sections[2].count)return SA3C_POSE_ERR_RANGE;
            const uint8_t *me=sec(c,2)+(size_t)mesh_i*c->sections[2].stride;
            uint16_t mat=rd16(me+2); if((uint32_t)mat>=c->sections[5].count)return SA3C_POSE_ERR_RANGE;
            const uint8_t *ma=sec(c,5)+(size_t)mat*c->sections[5].stride;
            SA3CDrawCommand cmd;
            cmd.node_index=ni;cmd.model_index=model;cmd.mesh_index=(uint16_t)mesh_i;cmd.material_index=mat;
            cmd.texture_slot=rd16(ma+12);cmd.source_primitive=rd16(me+12);cmd.first_index=rd32(me+4);cmd.index_count=rd32(me+8);cmd.world=world[ni];
            st->draw_calls++;st->indices+=cmd.index_count;st->triangles+=cmd.index_count/3u;
            if(visitor && visitor(&cmd,user)!=0)return SA3C_POSE_ERR_ARGUMENT;
        }
    }
    if(!(flags&SA3C_EVAL_BREAK)&&child!=SA3C_INVALID_U16){SA3CPoseResult r=emit_node(c,child,world,visitor,user,st,depth+1);if(r!=SA3C_POSE_OK)return r;}
    if(sibling!=SA3C_INVALID_U16)return emit_node(c,sibling,world,visitor,user,st,depth+1);
    return SA3C_POSE_OK;
}

SA3CPoseResult sa3c_render_emit(const SA3CCharacter *c,uint16_t root_node,const SA3CMatrix *world,size_t world_count,SA3CDrawVisitor visitor,void *user,SA3CRenderStats *stats){
    if(!c||!c->data||!world||!stats)return SA3C_POSE_ERR_ARGUMENT;
    if(world_count<c->sections[0].count||(uint32_t)root_node>=c->sections[0].count)return SA3C_POSE_ERR_RANGE;
    memset(stats,0,sizeof(*stats));return emit_node(c,root_node,world,visitor,user,stats,0);
}
