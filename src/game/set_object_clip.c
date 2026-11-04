#include "set_object_clip.h"
#include <math.h>
int sa3ds_set_position_in_range(const float p[3],const float r[3],float threshold,int*out){
 if(!p||!r||!out||!isfinite(threshold))return 0;
 double d[3];for(unsigned k=0;k<3;k++){
  if(!isfinite(p[k])||!isfinite(r[k]))return 0;
  d[k]=(double)p[k]-r[k];
 }
 *out=0;double sum=d[0]*d[0];if(sum>threshold)return 1;
 sum+=d[1]*d[1];if(sum>threshold)return 1;
 sum+=d[2]*d[2];*out=sum<=threshold;return 1;
}
int sa3ds_set_unload_distance(float load,float*out){
 if(!out||!isfinite(load))return 0;
 float value=(float)((double)load+100);if(!isfinite(value))return 0;*out=value;return 1;
}
int sa3ds_set_object_clip(const SA3DSSetClipInput*i,uint8_t*out){
 if(!i||!out)return 0;
 *out=0;
 if(i->has_set_data&&(i->set_flags&8))return 1;
 float threshold=i->has_set_data?i->unload_distance_squared:168100.f;
 if(!isfinite(threshold))return 0;
 if(threshold==0)return 1;
 const SA3DSSetClipReference*references[]={&i->camera,&i->player0,&i->player1};
 for(unsigned n=0;n<3;n++){
  if(!references[n]->present||(n==2&&i->player1_character!=0))continue;
  int inside;if(!sa3ds_set_position_in_range(i->position,references[n]->position,n==2?1600.f:threshold,&inside))return 0;
  if(inside)return 1;
 }
 *out=1;return 1;
}
