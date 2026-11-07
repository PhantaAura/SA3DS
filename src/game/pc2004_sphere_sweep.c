#include "pc2004_sphere_sweep.h"
#include <math.h>
#include <string.h>
#define BAMS_TO_RAD (6.28318530717958647692/65536.0)
typedef struct {float m[16];} Mat; /*sa3c/SA Tools element order*/
static void identity(Mat*a){memset(a,0,sizeof(*a));a->m[0]=a->m[5]=a->m[10]=a->m[15]=1;}
static void rotx(Mat*a,float x){
 float*m=a->m,s=sinf(x),c=cosf(x),m21=m[4],m22=m[5],m23=m[6],m24=m[7];
 m[4]=s*m[8]+m21*c;m[5]=s*m[9]+m22*c;m[6]=m23*c+s*m[10];m[7]=s*m[11]+m24*c;
 m[8]=c*m[8]-m21*s;m[9]=c*m[9]-m22*s;m[10]=c*m[10]-m23*s;m[11]=c*m[11]-m24*s;
}
static void rotz(Mat*a,float z){
 float*m=a->m,s=sinf(z),c=cosf(z),m11=m[0],m12=m[1],m13=m[2],m14=m[3];
 m[0]=s*m[4]+m11*c;m[1]=s*m[5]+m12*c;m[2]=s*m[6]+m13*c;m[3]=s*m[7]+m14*c;
 m[4]=c*m[4]-m11*s;m[5]=c*m[5]-m12*s;m[6]=c*m[6]-m13*s;m[7]=c*m[7]-m14*s;
}
static void apply(const Mat*a,const float p[3],float o[3]){ /*781670 njCalcPoint*/
 float x=p[0],y=p[1],z=p[2];
 o[0]=a->m[0]*x+a->m[4]*y+a->m[8]*z+a->m[12];
 o[1]=a->m[1]*x+a->m[5]*y+a->m[9]*z+a->m[13];
 o[2]=a->m[2]*x+a->m[6]*y+a->m[10]*z+a->m[14];
}
static void apply_inverse(const Mat*a,const float p[3],float o[3]){ /*787630 then781670: rotation only*/
 float x=p[0],y=p[1],z=p[2];
 o[0]=a->m[0]*x+a->m[1]*y+a->m[2]*z;
 o[1]=a->m[4]*x+a->m[5]*y+a->m[6]*z;
 o[2]=a->m[8]*x+a->m[9]*y+a->m[10]*z;
}
void sa3ds_pc2004_normal_angles(float x,float y,float z,int32_t*ax,int32_t*az){
 /*4B8E80: near +-Z poles and tiny X/Y with a signed Z give fixed angles.*/
 if(z>.99996f){*ax=0x4000;*az=0;return;}
 if(fabsf(y)<=.002f&&fabsf(x)<=.002f&&z>0){*ax=0x4000;*az=0;return;}
 if(z< -.99996f){*ax=0xc000;*az=0;return;}
 if(fabsf(y)<=.002f&&fabsf(x)<=.002f&&z<0){*ax=0xc000;*az=0;return;}
 *ax=(int32_t)(asin((double)z)*65536.*.15915497620314795);
 *az=(int32_t)(atan2((double)x,(double)y)*65536.*-.15915497620314795);
}
static void normalize_or_up(float n[3]){ /*4509D0: zero -> (0,1,0)*/
 float l=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
 if(l==0){n[0]=0;n[1]=1;n[2]=0;return;}
 float k=1.f/l;n[0]*=k;n[1]*=k;n[2]*=k;
}
int sa3ds_pc2004_sweep_triangle(const SA3DSSweepTriangle*t,const float s[3],const float d[3],float r,SA3DSSweepHit*out){
 if(!t||!s||!d||!out||!(r>0))return 0; /*453820 rejects radius<=0*/
 const float*o=t->v[0];
 float a[3],b[3],c[3],cen[3];
 for(unsigned k=0;k<3;k++){a[k]=0;b[k]=t->v[1][k]-o[k];c[k]=t->v[2][k]-o[k];}
 for(unsigned k=0;k<3;k++)cen[k]=(b[k]+0.f+c[k])/3.f; /*(v1-v0 + 0 + v2-v0)/3*/
 for(unsigned k=0;k<3;k++){ /*inflate1.04 about the centroid*/
  a[k]=(a[k]-cen[k])*1.04f+cen[k];b[k]=(b[k]-cen[k])*1.04f+cen[k];c[k]=(c[k]-cen[k])*1.04f+cen[k];
 }
 /*450810 cross(b-a, a-c) ordering*/
 float n[3]={(a[1]-c[1])*(b[2]-a[2])-(a[2]-c[2])*(b[1]-a[1]),
             (a[2]-c[2])*(b[0]-a[0])-(a[0]-c[0])*(b[2]-a[2]),
             (a[0]-c[0])*(b[1]-a[1])-(a[1]-c[1])*(b[0]-a[0])};
 normalize_or_up(n);
 int32_t ax,az;sa3ds_pc2004_normal_angles(n[0],n[1],n[2],&ax,&az);
 Mat m;identity(&m);
 if(ax)rotx(&m,(float)((uint16_t)(-ax)*BAMS_TO_RAD));
 if(az)rotz(&m,(float)((uint16_t)(-az)*BAMS_TO_RAD));
 float la[3],lb[3],lc[3],ls[3],ld[3],rs[3]={s[0]-o[0],s[1]-o[1],s[2]-o[2]};
 apply(&m,a,la);apply(&m,b,lb);apply(&m,c,lc);apply(&m,rs,ls);apply(&m,d,ld);
 float length=sqrtf(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]),step=r*.5f;
 float signed_r=r;int flip=(t->flags&0x800000u)&&ls[1]<0;if(flip)signed_r=-r;
 float dir[3]={ld[0],ld[1],ld[2]},dl=sqrtf(dir[0]*dir[0]+dir[1]*dir[1]+dir[2]*dir[2]);
 if(dl==0)dir[0]=dir[1]=dir[2]=0;else for(unsigned k=0;k<3;k++)dir[k]=dir[k]*(1.f/dl)*step;
 float p[3]={ls[0],ls[1],ls[2]},travelled=0;int hit=0;
 for(;;){
  if(length<=travelled){p[0]=ls[0]+ld[0];p[1]=ls[1]+ld[1];p[2]=ls[2]+ld[2];}
  int more=length>travelled;
  if(!(fabsf(p[1])>r)){ /*within the slab: XZ cross-sign test*/
   float d4=(p[0]-la[0])*(lb[2]-la[2])-(p[2]-la[2])*(lb[0]-la[0]);
   float dc=(p[0]-lb[0])*(lc[2]-lb[2])-(p[2]-lb[2])*(lc[0]-lb[0]);
   float d8=(p[0]-lc[0])*(la[2]-lc[2])-(p[2]-lc[2])*(la[0]-lc[0]);
   if(!(d4*dc<0||dc*d8<0||d8*d4<0)){hit=1;break;}
  }
  if(!more)break;
  for(unsigned k=0;k<3;k++)p[k]+=dir[k];
  travelled+=step;
 }
 if(!hit)return 0;
 SA3DSSweepHit h;memset(&h,0,sizeof(h));
 for(unsigned k=0;k<3;k++){h.normal[k]=n[k];h.signed_normal[k]=flip?-n[k]:n[k];}
 h.angle_x=ax;h.angle_z=az;h.start_height=ls[1];h.end_height=ls[1]+ld[1];
 float q[3],w[3];
 q[0]=ls[0];q[1]=0;q[2]=ls[2];apply_inverse(&m,q,w);for(unsigned k=0;k<3;k++)h.plane_point[k]=w[k]+o[k];
 q[1]=signed_r;apply_inverse(&m,q,w);for(unsigned k=0;k<3;k++)h.corrected[k]=w[k]+o[k];
 q[0]=ld[0];q[1]=0;q[2]=ld[2];apply_inverse(&m,q,h.delta_xz);
 q[0]=0;q[1]=ld[1];q[2]=0;apply_inverse(&m,q,h.delta_y);
 *out=h;return 1;
}
int sa3ds_pc2004_sweep(const SA3DSSweepTriangle*t,size_t count,const float s[3],const float d[3],float r,
 SA3DSSweepHit*chosen,int*index){
 int hits=0,best=-1;SA3DSSweepHit h,keep;memset(&keep,0,sizeof(keep));
 for(size_t i=0;i<count&&hits<16;i++){
  if(!sa3ds_pc2004_sweep_triangle(&t[i],s,d,r,&h))continue;
  if(best<0||h.end_height<keep.end_height){keep=h;best=hits;}
  hits++;
 }
 if(hits&&chosen)*chosen=keep;
 if(index)*index=best;
 return hits;
}
