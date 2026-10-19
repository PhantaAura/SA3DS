#include "pc2004_collision_work.h"

#include <string.h>

void sa1_pc2004_collision_work_init(
    SA1PC2004CollisionWork *work,
    const SA1SteamPlayerParameters *p,
    SA1Vec3 pos,
    SA1Vec3 speed,
    int32_t angx,
    int32_t angz) {
    if (!work || !p) return;
    memset(work, 0, sizeof(*work));
    work->radius = p->height * 0.5f;
    work->pos = pos;
    work->spd = speed;
    work->tnorm = (SA1Vec3){0.0f, 1.0f, 0.0f};
    work->angx = angx;
    work->angz = angz;
}

void sa1_pc2004_collision_work_mirror_static_contact(
    SA1PC2004CollisionWork *work,
    const SA1Contact *contact,
    uint16_t selected_index) {
    if (!work || !contact || !contact->valid) return;
    work->find_count = 1;
    work->selected_nmb = selected_index;
    work->normal = contact->normal;
    work->onpoly = contact->point;
}
