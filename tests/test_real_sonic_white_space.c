#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"

static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rdf(const uint8_t *p){uint32_t u=rd32(p);float f;memcpy(&f,&u,4);return f;}
static unsigned char *read_all(const char *path,size_t *size){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0){fclose(f);return NULL;}unsigned char*p=malloc((size_t)n);if(!p){fclose(f);return NULL;}if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;}

typedef struct { const SA3CCharacter *c; float minv[3],maxv[3]; uint32_t commands; } BoundsCtx;
static void xform(const SA3CMatrix*m,float x,float y,float z,float*out){out[0]=x*m->m[0]+y*m->m[4]+z*m->m[8]+m->m[12];out[1]=x*m->m[1]+y*m->m[5]+z*m->m[9]+m->m[13];out[2]=x*m->m[2]+y*m->m[6]+z*m->m[10]+m->m[14];}
static int visit(const SA3CDrawCommand *cmd,void *user){
    BoundsCtx*b=(BoundsCtx*)user;const SA3CCharacter*c=b->c;
    if(cmd->texture_slot>=c->sections[6].count||cmd->index_count%3u)return 1;
    const uint8_t*ib=c->data+c->sections[4].offset;const uint8_t*vb=c->data+c->sections[3].offset;
    for(uint32_t i=0;i<cmd->index_count;i++){
        uint16_t vi=rd16(ib+(size_t)(cmd->first_index+i)*2u);if((uint32_t)vi>=c->sections[3].count)return 1;
        const uint8_t*v=vb+(size_t)vi*c->sections[3].stride;float p[3];xform(&cmd->world,rdf(v),rdf(v+4),rdf(v+8),p);
        for(int k=0;k<3;k++){if(!isfinite(p[k]))return 1;if(p[k]<b->minv[k])b->minv[k]=p[k];if(p[k]>b->maxv[k])b->maxv[k]=p[k];}
        float u=sa3c_basic_uv_normalize(rdf(v+24)), vv=sa3c_basic_uv_normalize(rdf(v+28));if(!isfinite(u)||!isfinite(vv))return 1;
    }
    b->commands++;return 0;
}

int main(int argc,char**argv){
    if(argc!=2){fprintf(stderr,"usage: %s sonic.sa3c\n",argv[0]);return 2;}
    size_t sz=0;unsigned char*bytes=read_all(argv[1],&sz);if(!bytes)return 2;
    SA3CCharacter c;SA3CResult cr=sa3c_character_open(&c,bytes,sz);if(cr!=SA3C_OK||sa3c_character_validate_references(&c)!=SA3C_OK){free(bytes);return 1;}
    size_t n=c.sections[0].count;SA3CTransform*local=calloc(n,sizeof(*local));SA3CTransform*local2=calloc(n,sizeof(*local2));SA3CMatrix*world=calloc(n,sizeof(*world));if(!local||!local2||!world)return 2;
    uint16_t root=0xffff;SA3CPoseResult r=sa3c_pose_sample_action(&c,0,0.0f,1,local,n,&root);if(r!=SA3C_POSE_OK){fprintf(stderr,"sample0 %s\n",sa3c_pose_result_name(r));return 1;}
    if(root==0xffff)return 1;
    r=sa3c_pose_build_world(&c,root,local,n,world,n);if(r!=SA3C_POSE_OK){fprintf(stderr,"world %s\n",sa3c_pose_result_name(r));return 1;}
    BoundsCtx bc={.c=&c,.minv={FLT_MAX,FLT_MAX,FLT_MAX},.maxv={-FLT_MAX,-FLT_MAX,-FLT_MAX},.commands=0};SA3CRenderStats st;
    r=sa3c_render_emit(&c,root,world,n,visit,&bc,&st);if(r!=SA3C_POSE_OK){fprintf(stderr,"emit %s\n",sa3c_pose_result_name(r));return 1;}
    if(!st.draw_calls||st.draw_calls!=bc.commands||!st.triangles||st.indices!=st.triangles*3u)return 1;
    for(int k=0;k<3;k++)if(!(bc.maxv[k]>bc.minv[k]))return 1;
    /* The real animation must change at least one local channel at a fractional frame. */
    uint16_t root2=0xffff;r=sa3c_pose_sample_action(&c,0,1.5f,1,local2,n,&root2);if(r!=SA3C_POSE_OK||root2!=root)return 1;
    int changed=0;for(size_t i=0;i<n;i++)if(memcmp(&local[i],&local2[i],sizeof(local[i]))!=0){changed=1;break;}if(!changed)return 1;
    /* Empty ACTION slots remain explicit, not silently substituted. */
    int empty_seen=0;for(uint32_t ai=0;ai<c.sections[7].count;ai++){const uint8_t*a=c.data+c.sections[7].offset+(size_t)ai*c.sections[7].stride;if(rd16(a+2)==0xffff){SA3CPoseResult er=sa3c_pose_sample_action(&c,(uint16_t)ai,0,1,local2,n,NULL);if(er!=SA3C_POSE_ERR_EMPTY_ACTION)return 1;empty_seen=1;break;}}if(!empty_seen)return 1;
    if(fabsf(sa3c_basic_uv_normalize(255.0f)-1.0f)>1e-6f)return 1;
    printf("real Sonic white-space pose/render submission: PASS\n");
    printf("action0 root=%u visited=%u visible_nodes=%u draws=%u triangles=%u bounds=[%.6f %.6f %.6f]-[%.6f %.6f %.6f]\n",root,st.visited_nodes,st.visible_model_nodes,st.draw_calls,st.triangles,bc.minv[0],bc.minv[1],bc.minv[2],bc.maxv[0],bc.maxv[1],bc.maxv[2]);
    free(world);free(local2);free(local);free(bytes);return 0;
}
