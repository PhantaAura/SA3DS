#include "whale_trigger_assets.h"
#include "whale_trigger.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 float center[3]={0,0,0};SA3DSWhaleContactPlayer p[2]={{{45,0,0},1,1},{{0,0,0},1,1}};uint8_t inside;
 assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==2); /*Strict boundary.*/
 p[0].center[0]=nextafterf(45,0);assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==1);
 p[0].ccl_present=0;assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==2);
 p[0].ccl_present=1;p[0].task_present=0;assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==2);
 p[1].task_present=0;assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==0);
 p[0].task_present=1;p[0].center[0]=nextafterf(1,0);p[0].center[1]=.00033f;
 assert((double)p[0].center[0]*p[0].center[0]+(double)p[0].center[1]*p[0].center[1]<1);
 assert(sa3ds_whale_trigger_inside(center,1,p,&inside)&&inside==0); /*Float length sum rounds to1.*/
 p[0].center[0]=p[0].center[1]=0;
 assert(sa3ds_whale_trigger_inside(center,45,p,&inside)&&inside==1);
 SA3DSWhaleTrigger task={0};SA3DSWhaleTriggerInput input={.inside_player=inside};SA3DSWhaleTriggerResult result;
 assert(sa3ds_whale_summon_tick(&task,&input,&result)&&task.action==1&&!result.summon);
 input.inside_player=0;assert(sa3ds_whale_summon_tick(&task,&input,&result)&&result.summon);
 assert(!sa3ds_whale_trigger_inside(center,-1,p,&inside));
 if(argc>1){
  FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);void*data=malloc(size);assert(data&&fread(data,1,size,f)==(size_t)size);fclose(f);
  SA3DSWhaleTriggerAssets a;int valid=sa3ds_whale_trigger_assets_open(&a,data,size);
  if(argc==2){free(data);return valid?1:0;}
  assert(valid&&a.radius==45&&a.load_distance_squared==160000);f=fopen(argv[2],"wb");assert(f);
  for(uint32_t i=0;i<a.count;i++){
   SA3DSWhalePlacement row;assert(sa3ds_whale_trigger_placement(&a,i,&row));
   assert(fwrite(&row.act,4,1,f)==1&&fwrite(&row.source_row,4,1,f)==1&&fwrite(&row.type,2,1,f)==1&&fwrite(row.angles,2,3,f)==3&&fwrite(row.position,4,3,f)==3&&fwrite(row.parameters,4,3,f)==3);
  }
  SA3DSWhalePlacement row;assert(!sa3ds_whale_trigger_placement(&a,a.count,&row));assert(!fclose(f));free(data);
 }
 puts("Whale trigger assets/contact: PASS strict sphere/ULP boundary,float length-store distinction,first-player task/CCL presence and delayed summon composition; native clip/task executor separate");
}
