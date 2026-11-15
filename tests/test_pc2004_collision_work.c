#include "pc2004_collision_work.h"
#include "steam_player_parameters.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static int nearf(float a, float b) { return fabsf(a - b) < 1e-6f; }

int main(void) {
    assert(sizeof(SA1PC2004CollisionWork) == 0x84);
    assert(offsetof(SA1PC2004CollisionWork, radius) == 0x00);
    assert(offsetof(SA1PC2004CollisionWork, pos) == 0x04);
    assert(offsetof(SA1PC2004CollisionWork, spd) == 0x10);
    assert(offsetof(SA1PC2004CollisionWork, tnorm) == 0x1C);
    assert(offsetof(SA1PC2004CollisionWork, find_count) == 0x28);
    assert(offsetof(SA1PC2004CollisionWork, selected_nmb) == 0x2A);
    assert(offsetof(SA1PC2004CollisionWork, yt) == 0x2C);
    assert(offsetof(SA1PC2004CollisionWork, yb) == 0x30);
    assert(offsetof(SA1PC2004CollisionWork, angx) == 0x34);
    assert(offsetof(SA1PC2004CollisionWork, angz) == 0x38);
    assert(offsetof(SA1PC2004CollisionWork, normal) == 0x3C);
    assert(offsetof(SA1PC2004CollisionWork, normal2) == 0x48);
    assert(offsetof(SA1PC2004CollisionWork, onpoly) == 0x54);
    assert(offsetof(SA1PC2004CollisionWork, pshbk) == 0x60);
    assert(offsetof(SA1PC2004CollisionWork, anaspdh) == 0x6C);
    assert(offsetof(SA1PC2004CollisionWork, anaspdv) == 0x78);

    SA1PC2004CollisionWork w;
    SA1Vec3 pos = {1.0f, 2.0f, 3.0f};
    SA1Vec3 spd = {4.0f, 5.0f, 6.0f};
    sa1_pc2004_collision_work_init(&w, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE,
                                   pos, spd, 0x1234, -0x2345);
    assert(nearf(w.radius, 5.0f)); /* Sonic height 10 -> retail csts radius 5. */
    assert(nearf(w.pos.x, 1.0f) && nearf(w.pos.y, 2.0f) && nearf(w.pos.z, 3.0f));
    assert(nearf(w.spd.x, 4.0f) && nearf(w.spd.y, 5.0f) && nearf(w.spd.z, 6.0f));
    assert(nearf(w.tnorm.x, 0.0f) && nearf(w.tnorm.y, 1.0f) && nearf(w.tnorm.z, 0.0f));
    assert(w.angx == 0x1234 && w.angz == -0x2345);
    assert(w.find_count == 0 && w.selected_nmb == 0);

    SA1Contact c = {0};
    c.valid = 1;
    c.point = (SA1Vec3){7.0f, 8.0f, 9.0f};
    c.normal = (SA1Vec3){0.0f, 0.8f, 0.6f};
    sa1_pc2004_collision_work_mirror_static_contact(&w, &c, 3);
    assert(w.find_count == 1 && w.selected_nmb == 3);
    assert(nearf(w.normal.y, 0.8f) && nearf(w.onpoly.z, 9.0f));
    /* Unknown retail solver outputs remain zero instead of being fabricated. */
    assert(nearf(w.normal2.x, 0.0f) && nearf(w.pshbk.x, 0.0f));
    assert(nearf(w.anaspdh.x, 0.0f) && nearf(w.anaspdv.x, 0.0f));

    SA1Vec3 up={0,1,0}, gravity={0,-1,0};
    assert(sa1_pc2004_collision_ground_side(up,(SA1Vec3){0.8660254f,0.5f,0},gravity,0));
    assert(!sa1_pc2004_collision_ground_side(up,(SA1Vec3){0.9238795f,0.3826834f,0},gravity,0));
    assert(sa1_pc2004_collision_ground_side(up,(SA1Vec3){1,0,0},gravity,1));
    assert(sa1_pc2004_collision_ground_side((SA1Vec3){1,0,0},up,gravity,0));
    assert(!sa1_pc2004_collision_ground_side(up,(SA1Vec3){1,0,0},gravity,0));

    SA1Vec3 projected = sa1_pc2004_collision_support_position(
        (SA1Vec3){3,8,1}, (SA1Vec3){2,2,3}, (SA1Vec3){0,2,0},
        (SA1Vec3){-2,2,0});
    assert(nearf(projected.x,6.5f) && nearf(projected.y,8.5f) &&
           nearf(projected.z,4.0f));

    puts("PC 2004 collision-work layout/init tests: PASS");
    return 0;
}
