#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "surface_frame.h"
#include "stage.h"

static int near(float a, float b, float eps) { return fabsf(a-b) <= eps; }

int main(void) {
    SA1SurfaceFrame flat = sa1_surface_frame_make(
        (SA1Vec3){0,1,0}, (SA1Vec3){0,0,1});
    assert(flat.valid);
    assert(near(flat.forward.z, 1.0f, 1e-5f));
    assert(near(flat.right.x, 1.0f, 1e-5f));

    SA1Vec3 slope_n = sa1_vec3_normalize((SA1Vec3){0,1,-1});
    SA1SurfaceFrame slope = sa1_surface_frame_make(slope_n, (SA1Vec3){0,0,1});
    assert(slope.valid);
    assert(fabsf(sa1_vec3_dot(slope.forward, slope.up)) < 1e-5f);
    assert(fabsf(sa1_vec3_dot(slope.right, slope.up)) < 1e-5f);
    assert(fabsf(sa1_vec3_dot(slope.forward, slope.right)) < 1e-5f);

    SA1SurfaceFrame wall = sa1_surface_frame_make(
        (SA1Vec3){0,0,-1}, (SA1Vec3){0,1,0});
    assert(wall.valid);
    assert(near(wall.up.z, -1.0f, 1e-5f));

    SA1SurfaceFrame ceiling = sa1_surface_frame_make(
        (SA1Vec3){0,-1,0}, (SA1Vec3){0,0,-1});
    assert(ceiling.valid);
    assert(near(ceiling.up.y, -1.0f, 1e-5f));

    SA1Contact bottom = sa3ds_movement_lab_contact(
        0, (SA1Vec3){0,5,225}, (SA1Vec3){0,-1,0}, 2.35f);
    assert(bottom.valid);
    assert(bottom.normal.y > 0.99f);

    SA1Contact side = sa3ds_movement_lab_contact(
        0, (SA1Vec3){0,18,238}, (SA1Vec3){0,0,1}, 2.35f);
    assert(side.valid && side.surface_id == 100);
    assert(side.normal.z < -0.99f);

    SA1Contact top = sa3ds_movement_lab_contact(
        0, (SA1Vec3){0,31,225}, (SA1Vec3){0,1,0}, 2.35f);
    assert(top.valid && top.surface_id == 100);
    assert(top.normal.y < -0.99f);

    printf("general surface frame/contact tests: PASS\n");
    return 0;
}
