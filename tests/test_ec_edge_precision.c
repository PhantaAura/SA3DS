/* Source-neutral numeric regression for the port's capsule sweep, not retail
 * collision authority. Include actual implementation to exercise private primitive. */
#include "../src/game/emerald_coast_collision.c"
#include <assert.h>
#include <stdio.h>

int main(void) {
    unsigned state=12345, trials=0, misses=0;
    for(unsigned i=0;i<4096;i++) {
        state=state*1664525u+1013904223u;
        float ax=(float)((int)(state%201)-100);
        state=state*1664525u+1013904223u;
        float az=(float)((int)(state%201)-100);
        state=state*1664525u+1013904223u;
        float dx=(float)(50+state%101);
        state=state*1664525u+1013904223u;
        float dz=(float)((int)(state%201)-100);
        SA1Vec3 a={ax,6,az},b={ax+dx,6,az+dz};
        SA1Vec3 edge=sa1_vec3_sub(b,a);
        SA1Vec3 radial=sa1_vec3_normalize((SA1Vec3){-edge.z,0,edge.x});
        SA1Vec3 midpoint=sa1_vec3_add(a,sa1_vec3_scale(edge,.4f));
        SA1Vec3 previous=sa1_vec3_add(midpoint,sa1_vec3_scale(radial,5));
        SA1Vec3 current=sa1_vec3_add(midpoint,sa1_vec3_scale(radial,4.5f));
        float q;SA1Vec3 normal;
        trials++;
        if(!sweep_edge(previous,current,a,b,5,&q,&normal)) {
            if(misses++<3)fprintf(stderr,"missed inward boundary sweep a=(%.0f,%.0f) edge=(%.0f,%.0f)\n",ax,az,dx,dz);
        } else {
            assert(q>=0&&q<.001f);
            assert(sa1_vec3_dot(sa1_vec3_sub(current,previous),normal)<0);
        }
        /* Moving away from a near-boundary contact must stay unblocked. */
        SA1Vec3 outward=sa1_vec3_add(midpoint,sa1_vec3_scale(radial,5.5f));
        assert(!sweep_edge(previous,outward,a,b,5,&q,&normal));
    }
    assert(misses==0);
    printf("EC edge precision: PASS (%u synthetic angled boundary sweeps; outward motion preserved)\n",trials);
    return 0;
}
