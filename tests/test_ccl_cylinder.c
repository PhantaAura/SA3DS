#include "ccl_cylinder.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int near(double a,double b){return fabs(a-b)<1e-4;}
static void setup(SA3DSCCLBody*p,SA3DSCCLShape*ps,SA3DSCCLBody*c,SA3DSCCLShape*cs){
 sa3ds_ccl_body_init(p,1);sa3ds_ccl_body_init(c,2);p->hits.has_ccl=c->hits.has_ccl=1;
 p->rules=(SA3DSCCLRules){1,0,0,0};c->rules=(SA3DSCCLRules){0,0,0,0};
 *ps=(SA3DSCCLShape){7,0,0,0,{0,0,0},4};*cs=(SA3DSCCLShape){1,1,0,0,{0,5,0},5};
}
int main(void){
 SA3DSCCLBody p,c;SA3DSCCLShape ps,cs;SA3DSCCLPrimaryExtent ext={4,0.65f};int tilt;
 /*Landing on top: feet just below the cylinder top (ctop=10).*/
 setup(&p,&ps,&c,&cs);SA1Vec3 pos={1,9.5f,0};
 assert(sa3ds_ccl_cylinder_contact(&p,&ps,0,&ext,&c,&cs,0,5,1,&pos,&tilt)==1&&near(pos.y,9.99)&&tilt&&(p.status&0x20)&&(c.status&0x10)&&(p.outer_flags&8)&&(c.outer_flags&2));
 assert((p.outer_flags&1)&&c.cached_other_id==1&&c.cached_self_shape==0);
 /*Side: centred at mid height, 8 units away (pa+ca=9): pushed by (1-0.1)/2.*/
 setup(&p,&ps,&c,&cs);pos=(SA1Vec3){8,1,0};
 assert(sa3ds_ccl_cylinder_contact(&p,&ps,0,&ext,&c,&cs,0,5,1,&pos,&tilt)==2&&near(pos.x,8.45)&&near(p.direction_x,1)&&(p.outer_flags&4));
 /*Head bump: ptop just above cbot (0).*/
 setup(&p,&ps,&c,&cs);pos=(SA1Vec3){0,-7,0};
 assert(sa3ds_ccl_cylinder_contact(&p,&ps,0,&ext,&c,&cs,0,5,1,&pos,&tilt)==3&&near(pos.y,-7-1+0.01)&&(p.status&0x10));
 /*Miss and no-push.*/
 setup(&p,&ps,&c,&cs);pos=(SA1Vec3){20,1,0};assert(sa3ds_ccl_cylinder_contact(&p,&ps,0,&ext,&c,&cs,0,5,1,&pos,&tilt)==0);
 setup(&p,&ps,&c,&cs);p.rules.push_receive=0;pos=(SA1Vec3){8,1,0};
 assert(sa3ds_ccl_cylinder_contact(&p,&ps,0,&ext,&c,&cs,0,5,1,&pos,&tilt)==2&&pos.x==8);
 printf("CCL cylinder 41CCD0: PASS top landing (ctop-0.01, ground 0x20, tilt clear), side push n*(pen-0.1)/2, head bump, miss, push gate, contact records\n");
}
