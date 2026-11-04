#include "whale_ccl.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(SA3DSWhaleCCLAssets*a){
 SA3DSWhaleCCL w;SA1Vec3 origin={0,0,0},tip={80,0,0};
 assert(sa3ds_whale_ccl_init(&w,a,8,origin));
 assert(w.body.outer_flags==0x8040&&w.body.hits.has_ccl&&w.body.cached_other_shape==-1);
 assert(!(w.shapes[0].attributes&0x10)&&(w.shapes[1].attributes&0x10));
 assert(sa3ds_whale_ccl_publish(&w,origin,tip)&&w.shapes[0].center.x==80&&w.shapes[1].center.x==0);
 SA3DSCCLBody player;sa3ds_ccl_body_init(&player,1);player.task_position=origin;player.broad_radius=10;
 player.rules=(SA3DSCCLRules){1,16,32,0x40000};
 SA3DSCCLShape sphere={.form=0,.push=0x70,.damage=0,.attributes=0x400,.center=origin,.radius=1};SA3DSCCLContactResult r;
 assert(sa3ds_ccl_sphere_contact(&player,&sphere,0,&w.body,&w.shapes[1],1,&r)&&!r.overlap&&!r.eligible);
 sa3ds_whale_ccl_emerge(&w);assert(!(w.shapes[1].attributes&0x10));
 assert(sa3ds_ccl_sphere_contact(&player,&sphere,0,&w.body,&w.shapes[1],1,&r)&&r.overlap&&!r.pushed_first&&!r.pushed_second);
 assert(w.body.status==8&&player.status==8&&w.body.cached_self_shape==1);
 sa3ds_ccl_body_clear(&w.body,w.shapes,2);sa3ds_ccl_body_clear(&player,&sphere,1);
 assert(!w.body.status&&!w.body.hit_count);
 player.task_position=sphere.center=tip;
 assert(sa3ds_ccl_sphere_contact(&player,&sphere,0,&w.body,&w.shapes[0],0,&r)&&r.overlap);
 assert(player.status==12&&(sphere.damage&0x10)&&w.body.status==8&&!(w.shapes[0].damage&0x10));
 assert(w.body.cached_self_shape==0&&!r.pushed_first&&!r.pushed_second);
 /*Publishing next frame must preserve prior hit/cache for actor consumption.*/
 assert(sa3ds_whale_ccl_publish(&w,(SA1Vec3){2,3,4},(SA1Vec3){81,9,7}));
 assert(w.body.status==8&&w.body.cached_other_id==1&&w.shapes[1].center.y==3&&w.shapes[0].center.y==9);
 SA3DSWhaleCCL before=w;assert(!sa3ds_whale_ccl_publish(&w,origin,(SA1Vec3){NAN,0,0}));assert(!memcmp(&before,&w,sizeof(w)));
}
int main(int argc,char**argv){
 /*Synthetic profile: owner data is supplied only as an external argument.*/
 SA3DSWhaleCCLAssets a={.shapes={{.push=0x70,.damage=0x21,.attributes=0x420,.radius=3},{.push=0x70,.radius=4}},.rules={2,32,64,0x80000},.broad_radius=150};
 if(argc==1){check(&a);puts("Whale CCL: PASS separate tip/body spheres, emergence gating, primary damage, secondary contact, no category push, retained prior hits and atomic invalid input");return 0;}
 FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);
 int valid=sa3ds_whale_ccl_assets_open(&a,b,n);free(b);
 if(argc==2)return valid?1:0;
 assert(valid);check(&a);f=fopen(argv[2],"wb");assert(f);
 for(unsigned k=0;k<2;k++){SA3DSCCLShape*s=&a.shapes[k];assert(fwrite(&s->form,1,1,f)==1&&fwrite(&s->push,1,1,f)==1&&fwrite(&s->damage,1,1,f)==1&&fwrite(&s->attributes,4,1,f)==1&&fwrite(&s->radius,4,1,f)==1);}
 assert(fwrite(&a.rules.push_receive,1,1,f)==1&&fwrite(&a.rules.push_inhibit,1,1,f)==1&&fwrite(&a.rules.damage_receive,1,1,f)==1&&fwrite(&a.rules.excluded_attributes,4,1,f)==1&&fwrite(&a.broad_radius,4,1,f)==1);assert(!fclose(f));return 0;
}
