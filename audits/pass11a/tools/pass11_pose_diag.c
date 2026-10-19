/* SA3DS Pass11A: owner-side, read-only Ninja pose & draw diagnostic.
 * Does not distribute or write Sega-owned game assets. */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"

static uint16_t rd16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rdf(const uint8_t *p){uint32_t u=rd32(p);float v;memcpy(&v,&u,4);return v;}
static int read_binary(const char *path,uint8_t **buf,size_t *size){
    FILE *f=fopen(path,"rb"); if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);if(n<272 || n>128*1024*1024 || fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return 0;}
    if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return 0;}
    fclose(f);*buf=p;*size=(size_t)n;return 1;
}
typedef struct{const SA3CCharacter *c;float lo[3],hi[3];unsigned actual_vertices;}Ctx;
static int visit(const SA3CDrawCommand *cmd,void *usr){
    Ctx *x=(Ctx*)usr;const SA3CCharacter *c=x->c;
    const uint8_t *ii=c->data+c->sections[4].offset;const uint8_t *vv=c->data+c->sections[3].offset;
    if(cmd->first_index>c->sections[4].count||cmd->index_count>c->sections[4].count-cmd->first_index)return 1;
    for(uint32_t j=0;j<cmd->index_count;j++){
        uint16_t id=rd16(ii+(size_t)(cmd->first_index+j)*2u);
        if(id>=c->sections[3].count)return 1;
        const uint8_t *v=vv+(size_t)id*c->sections[3].stride;
        float a=rdf(v),b=rdf(v+4),d=rdf(v+8);
        float p[3]={a*cmd->world.m[0]+b*cmd->world.m[4]+d*cmd->world.m[8]+cmd->world.m[12],
                    a*cmd->world.m[1]+b*cmd->world.m[5]+d*cmd->world.m[9]+cmd->world.m[13],
                    a*cmd->world.m[2]+b*cmd->world.m[6]+d*cmd->world.m[10]+cmd->world.m[14]};
        for(int k=0;k<3;k++){if(!isfinite(p[k]))return 1;if(p[k]<x->lo[k])x->lo[k]=p[k];if(p[k]>x->hi[k])x->hi[k]=p[k];}
        x->actual_vertices++;
    }
    return 0;
}
static int node_csv(const char *path,const SA3CCharacter *c,const SA3CTransform *locals,const SA3CMatrix *world,uint16_t root){
    FILE*f=fopen(path,"wb");if(!f)return 0;
    fprintf(f,"node,root,model,child,sibling,flags,local_x,local_y,local_z,rot_x_rad,rot_y_rad,rot_z_rad,scale_x,scale_y,scale_z,world_pivot_x,world_pivot_y,world_pivot_z\n");
    for(uint32_t i=0;i<c->sections[0].count;i++){
        const uint8_t*r=c->data+c->sections[0].offset+(size_t)i*c->sections[0].stride;
        const SA3CTransform*t=&locals[i];const SA3CMatrix*m=&world[i];
        fprintf(f,"%u,%u,%u,%u,%u,0x%04x,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
        i,root,rd16(r),rd16(r+2),rd16(r+4),rd32(r+8),t->position.x,t->position.y,t->position.z,
        t->rotation.x,t->rotation.y,t->rotation.z,t->scale.x,t->scale.y,t->scale.z,m->m[12],m->m[13],m->m[14]);
    }
    if(fclose(f)!=0) return 0;
    return 1;
}
int main(int argc,char **argv){
    if(argc<5||argc>7){fprintf(stderr,"usage: %s sonic.sa3c ACTION_INDEX FRAME BIND(0|1) [--nodes output.csv]\n",argv[0]);return 2;}
    char*end=NULL;unsigned long action=strtoul(argv[2],&end,10);if(!end||*end||action>65535)return 2;
    end=NULL;double frame=strtod(argv[3],&end);if(!end||*end||!isfinite(frame)||frame<-100000||frame>100000)return 2;
    if(strcmp(argv[4],"0")&&strcmp(argv[4],"1"))return 2;
    if(argc==7&&strcmp(argv[5],"--nodes"))return 2;
    if(argc==6)return 2;
    uint8_t*bytes=NULL;size_t size=0;
    if(!read_binary(argv[1],&bytes,&size)){perror("sonic.sa3c read");return 2;}
    SA3CCharacter c;SA3CResult r=sa3c_character_open(&c,bytes,size);
    if(r==SA3C_OK)r=sa3c_character_validate_references(&c);
    if(r!=SA3C_OK){fprintf(stderr,"SA3C validation failed: %s\n",sa3c_result_name(r));free(bytes);return 1;}
    size_t n=c.sections[0].count;
    SA3CTransform*local=calloc(n,sizeof(*local));SA3CMatrix*world=calloc(n,sizeof(*world));
    if(!local||!world){free(local);free(world);free(bytes);return 2;}
    uint16_t root=0xffff;SA3CPoseResult pr=sa3c_pose_sample_action(&c,(uint16_t)action,(float)frame,0,local,n,&root);
    if(pr!=SA3C_POSE_OK){fprintf(stderr,"Action sample failed: %s\n",sa3c_pose_result_name(pr));goto err;}
    if(argv[4][0]=='1'){
        pr=sa3c_pose_bind(&c,local,n);if(pr!=SA3C_POSE_OK)goto err;
    }
    pr=sa3c_pose_build_world(&c,root,local,n,world,n);if(pr!=SA3C_POSE_OK)goto err;
    Ctx x={.c=&c,.lo={FLT_MAX,FLT_MAX,FLT_MAX},.hi={-FLT_MAX,-FLT_MAX,-FLT_MAX},.actual_vertices=0};
    SA3CRenderStats stats;
    pr=sa3c_render_emit(&c,root,world,n,visit,&x,&stats);
    if(pr!=SA3C_POSE_OK || stats.indices!=x.actual_vertices || !stats.triangles){fprintf(stderr,"emit invalid: %s\n",sa3c_pose_result_name(pr));goto err;}
    if(argc==7&&!node_csv(argv[6],&c,local,world,root)){fprintf(stderr,"failed writing node CSV\n");goto err;}
    printf("action,frame,bind,root,vertices,triangles,min_x,min_y,min_z,max_x,max_y,max_z\n");
    printf("%lu,%.3f,%s,%u,%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",action,frame,argv[4],root,stats.indices,stats.triangles,x.lo[0],x.lo[1],x.lo[2],x.hi[0],x.hi[1],x.hi[2]);
    free(local);free(world);free(bytes);return 0;
err:
    free(local);free(world);free(bytes);return 1;
}
