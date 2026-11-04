/*Differential oracle: preserved pre-WCA59 edge scan versus current production
 * scan, including tolerance/classification boundaries and optional owner data.
 * Include the production TU to benchmark its private edge pass without adding
 * a test-only API to the runtime. No owner geometry is embedded here.*/
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "../src/game/emerald_coast_collision.c"
static void legacy_contact_edges(SA3DSECCollision*s){
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        SA1Vec3 v[3]={t->a,t->b,t->c};
        for(unsigned e=0;e<3;e++){
            int smooth_wall_neighbor=0,support_neighbor=0;SA1Vec3 a=v[e],b=v[(e+1)%3];
            unsigned c=(unsigned)cell_z(s,(a.z+b.z)*.5f)*16u+(unsigned)cell_x(s,(a.x+b.x)*.5f);
            for(uint32_t q=s->cells[c];q<s->cells[c+1];q++){
                uint32_t j=s->references[q];SA3DSECTriangle*n=&s->triangles[j];if(i==j)continue;
                SA1Vec3 nv[3]={n->a,n->b,n->c};int found_a=0,found_b=0;
                for(unsigned k=0;k<3;k++){found_a|=same_point(a,nv[k]);found_b|=same_point(b,nv[k]);}
                if(!found_a||!found_b)continue;
                float ndot=sa1_vec3_dot(t->normal,n->normal);
                if(fabsf(t->normal.y)<=0.45f&&fabsf(n->normal.y)<=0.45f&&fabsf(ndot)>0.92f)smooth_wall_neighbor=1;
                if(t->normal.y>0.45f&&n->normal.y>0.45f&&ndot>0.9f)support_neighbor=1;
            }
            if(fabsf(t->normal.y)<=0.45f&&!smooth_wall_neighbor)t->wall_edge_mask|=(uint8_t)(1u<<e);
            if(t->normal.y>0.45f&&support_neighbor)t->support_edge_mask|=(uint8_t)(1u<<e);
        }
    }
}

static void reset_masks(SA3DSECCollision*s){
 for(uint32_t i=0;i<s->triangle_count;i++)s->triangles[i].wall_edge_mask=s->triangles[i].support_edge_mask=0;
}
static void compare(SA3DSECCollision*s,const char*label){
 SA3DSECCollision old=*s;size_t bytes=(size_t)s->triangle_count*sizeof(*s->triangles);
 old.triangles=malloc(bytes);assert(old.triangles);memcpy(old.triangles,s->triangles,bytes);
 double legacy_ms=0,optimized_ms=0;
 for(unsigned pass=0;pass<3;pass++){
  reset_masks(&old);clock_t start=clock();legacy_contact_edges(&old);legacy_ms+=(double)(clock()-start)*1000/CLOCKS_PER_SEC;
  reset_masks(s);start=clock();mark_contact_edges(s);optimized_ms+=(double)(clock()-start)*1000/CLOCKS_PER_SEC;
  assert(!memcmp(old.triangles,s->triangles,bytes));
 }
 printf("Edges %s: exact %u triangles; host legacy %.3f ms -> optimized %.3f ms (mean3; not hardware)\n",label,s->triangle_count,legacy_ms/3,optimized_ms/3);
 free(old.triangles);
}
static void thresholds(void){
 SA3DSECCollision s={0};SA3DSECTriangle triangles[48]={0};uint32_t references[48];
 s.triangles=triangles;s.triangle_count=48;s.references=references;s.reference_count=48;
 for(unsigned c=137;c<=256;c++)s.cells[c]=48; /*midpoint cell(8,8)*/
 const float ys[]={1,-1,0,.45f,-.45f,.450001f,-.450001f,.449999f};
 const float shifts[]={0,.000999f,.001001f};
 for(unsigned i=0;i<48;i++){
  references[i]=i;float y=ys[i%8],shift=shifts[(i/8)%3];
  triangles[i].normal=(SA1Vec3){(i&1?-1:1)*sqrtf(fmaxf(0,1-y*y)),y,0};
  triangles[i].a=(SA1Vec3){shift,0,0};triangles[i].b=(SA1Vec3){.5f+shift,0,0};triangles[i].c=(SA1Vec3){shift,0,.5f};
 }
 compare(&s,"class/tolerance/opposed/overlap");
}
int main(int argc,char**argv){
 assert(argc>=2&&(argc==2||(argc-2)%3==0));thresholds();
 FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
 void*bytes=malloc(n);assert(bytes&&fread(bytes,1,n,f)==(size_t)n);fclose(f);
 SA3DSECCollisionArchive archive;assert(sa3ds_ec_collision_archive_open(&archive,bytes,n));
 const float synthetic[3][3]={{0,0,0},{1,8192,-16384},{2,-32768,24576}};
 unsigned count=argc==2?3:(unsigned)(argc-2)/3;
 for(unsigned i=0;i<count;i++){
  unsigned act=argc==2?(unsigned)synthetic[i][0]:(unsigned)strtoul(argv[2+i*3],NULL,10);
  float x=argc==2?synthetic[i][1]:strtof(argv[3+i*3],NULL),z=argc==2?synthetic[i][2]:strtof(argv[4+i*3],NULL);
  SA3DSECCollision s;assert(sa3ds_ec_collision_open_archive_region(&s,&archive,act,x,z));
  char label[80];snprintf(label,sizeof(label),"act%u center(%g,%g)",act,x,z);compare(&s,label);sa3ds_ec_collision_destroy(&s);
 }
 free(bytes);puts("Contact edge optimization: PASS exact legacy masks/triangle bytes; contact equations unchanged");return 0;
}
