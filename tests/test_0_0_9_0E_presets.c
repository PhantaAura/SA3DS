#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"

typedef struct{uint32_t commands;}Ctx;
static int visit(const SA3CDrawCommand*c,void*u){Ctx*x=u;if(!c->index_count||c->index_count%3)return 1;x->commands++;return 0;}
static unsigned char*readall(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return 0;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0){fclose(f);return 0;}unsigned char*b=malloc((size_t)z);if(!b){fclose(f);return 0;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return 0;}fclose(f);*n=(size_t)z;return b;}
int main(int argc,char**argv){if(argc!=2)return 2;size_t sz;unsigned char*b=readall(argv[1],&sz);if(!b)return 2;SA3CCharacter c;if(sa3c_character_open(&c,b,sz)!=SA3C_OK||sa3c_character_validate_references(&c)!=SA3C_OK)return 1;const uint16_t slots[]={1,3,6,14,10,11,7};size_t n=c.sections[0].count;SA3CTransform*l=calloc(n,sizeof(*l));SA3CMatrix*w=calloc(n,sizeof(*w));if(!l||!w)return 2;for(size_t i=0;i<sizeof(slots)/sizeof(slots[0]);i++){uint16_t root=0xffff;if(sa3c_pose_sample_action(&c,slots[i],0.0f,1,l,n,&root)!=SA3C_POSE_OK||root==0xffff)return 1;if(sa3c_pose_build_world(&c,root,l,n,w,n)!=SA3C_POSE_OK)return 1;SA3CRenderStats st;Ctx ctx={0};if(sa3c_render_emit(&c,root,w,n,visit,&ctx,&st)!=SA3C_POSE_OK||!ctx.commands||ctx.commands!=st.draw_calls||!st.triangles)return 1;printf("slot=%u root=%u draws=%u triangles=%u\n",slots[i],root,st.draw_calls,st.triangles);}free(w);free(l);free(b);puts("0.0.9.0E core animation presets: PASS");return 0;}
