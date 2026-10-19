#include "sa3c_character.h"

#include <string.h>

#define SA3C_HEADER_SIZE 272u
#define SA3C_SECTION_COUNT 10u
#define SA3C_DIR_OFFSET 64u
#define SA3C_DIR_STRIDE 20u
#define SA3C_INVALID_U16 0xFFFFu

static const char k_tags[10][4] = {
    {'N','O','D','E'},{'M','O','D','L'},{'M','E','S','H'},{'V','R','T','X'},{'I','N','D','X'},
    {'M','A','T','L'},{'T','E','X','S'},{'A','N','I','M'},{'C','H','A','N'},{'K','E','Y','S'}
};
static const uint32_t k_strides[10] = {48,32,24,32,2,32,24,24,24,16};

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int tag_eq(const char a[4], const char b[4]) { return memcmp(a,b,4)==0; }
static int add_overflows_u32(uint32_t a, uint32_t b) { return a > UINT32_MAX - b; }

const SA3CSection *sa3c_character_section(const SA3CCharacter *c, const char tag[4]) {
    if (!c || !tag) return NULL;
    for (unsigned i=0;i<SA3C_SECTION_COUNT;i++) if (tag_eq(c->sections[i].tag,tag)) return &c->sections[i];
    return NULL;
}

const void *sa3c_character_section_data(const SA3CCharacter *c, const SA3CSection *s) {
    if (!c || !s || !c->data) return NULL;
    if ((size_t)s->offset + (size_t)s->size > c->size) return NULL;
    return c->data + s->offset;
}

SA3CResult sa3c_character_open(SA3CCharacter *out, const void *data, size_t size) {
    if (!out || !data) return SA3C_ERR_ARGUMENT;
    memset(out,0,sizeof(*out));
    if (size < SA3C_HEADER_SIZE) return SA3C_ERR_TRUNCATED;
    const uint8_t *p=(const uint8_t*)data;
    if (memcmp(p,"SA3C",4)!=0) return SA3C_ERR_MAGIC;
    if (rd16(p+4)!=0 || rd16(p+6)!=0) return SA3C_ERR_VERSION;
    if (rd32(p+8)!=SA3C_HEADER_SIZE || rd32(p+12)!=SA3C_SECTION_COUNT) return SA3C_ERR_HEADER;
    if (rd32(p+20)!=(uint32_t)size) return SA3C_ERR_HEADER;
    if (SA3C_DIR_OFFSET + SA3C_SECTION_COUNT*SA3C_DIR_STRIDE > SA3C_HEADER_SIZE) return SA3C_ERR_HEADER;

    out->data=p; out->size=size; out->flags=rd32(p+16); memcpy(out->source_sha256,p+24,32);
    for (unsigned i=0;i<SA3C_SECTION_COUNT;i++) {
        const uint8_t *d=p+SA3C_DIR_OFFSET+i*SA3C_DIR_STRIDE;
        SA3CSection *s=&out->sections[i];
        memcpy(s->tag,d,4);
        s->count=rd32(d+4); s->offset=rd32(d+8); s->stride=rd32(d+12); s->size=rd32(d+16);
        if (!tag_eq(s->tag,k_tags[i])) return SA3C_ERR_SECTION;
        if (s->stride!=k_strides[i]) return SA3C_ERR_SECTION;
        if (s->count && s->stride > UINT32_MAX / s->count) return SA3C_ERR_RANGE;
        if (s->size != s->count*s->stride) return SA3C_ERR_SECTION;
        if (s->offset < SA3C_HEADER_SIZE || add_overflows_u32(s->offset,s->size)) return SA3C_ERR_RANGE;
        if ((size_t)s->offset+(size_t)s->size > size) return SA3C_ERR_RANGE;
        if ((s->offset & 0xFu)!=0) return SA3C_ERR_SECTION;
        if (i>0) {
            const SA3CSection *prev=&out->sections[i-1];
            if (s->offset < prev->offset + prev->size) return SA3C_ERR_RANGE;
        }
    }
    return SA3C_OK;
}

static int valid_u16_ref(uint16_t v,uint32_t count) { return v==SA3C_INVALID_U16 || (uint32_t)v<count; }
static int valid_range(uint32_t first,uint32_t count,uint32_t total) { return first<=total && count<=total-first; }

SA3CResult sa3c_character_validate_references(const SA3CCharacter *c) {
    if (!c || !c->data) return SA3C_ERR_ARGUMENT;
    const SA3CSection *node=&c->sections[0], *modl=&c->sections[1], *mesh=&c->sections[2], *vrtx=&c->sections[3], *indx=&c->sections[4];
    const SA3CSection *matl=&c->sections[5], *texs=&c->sections[6], *anim=&c->sections[7], *chan=&c->sections[8], *keys=&c->sections[9];
    const uint8_t *nodes=c->data+node->offset, *models=c->data+modl->offset, *meshes=c->data+mesh->offset;
    const uint8_t *indices=c->data+indx->offset, *materials=c->data+matl->offset, *anims=c->data+anim->offset, *channels=c->data+chan->offset;

    for (uint32_t i=0;i<node->count;i++) {
        const uint8_t *r=nodes+i*node->stride;
        uint16_t mi=rd16(r), child=rd16(r+2), sibling=rd16(r+4);
        if (!valid_u16_ref(mi,modl->count) || !valid_u16_ref(child,node->count) || !valid_u16_ref(sibling,node->count)) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<modl->count;i++) {
        const uint8_t *r=models+i*modl->stride;
        if (!valid_range(rd32(r),rd32(r+4),mesh->count)) return SA3C_ERR_REFERENCE;
        if (!valid_range(rd32(r+8),rd32(r+12),vrtx->count)) return SA3C_ERR_REFERENCE;
        if (!valid_range(rd32(r+16),rd32(r+20),indx->count)) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<mesh->count;i++) {
        const uint8_t *r=meshes+i*mesh->stride;
        if ((uint32_t)rd16(r)>=modl->count || (uint32_t)rd16(r+2)>=matl->count) return SA3C_ERR_REFERENCE;
        if (!valid_range(rd32(r+4),rd32(r+8),indx->count)) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<indx->count;i++) {
        if ((uint32_t)rd16(indices+i*2)>=vrtx->count) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<matl->count;i++) {
        const uint8_t *r=materials+i*matl->stride;
        if ((uint32_t)rd16(r+12)>=texs->count) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<anim->count;i++) {
        const uint8_t *r=anims+i*anim->stride;
        uint16_t root=rd16(r+2);
        if (!valid_u16_ref(root,node->count)) return SA3C_ERR_REFERENCE;
        if (!valid_range(rd32(r+8),rd32(r+12),chan->count)) return SA3C_ERR_REFERENCE;
    }
    for (uint32_t i=0;i<chan->count;i++) {
        const uint8_t *r=channels+i*chan->stride;
        if ((uint32_t)rd16(r)>=node->count) return SA3C_ERR_REFERENCE;
        uint32_t pos_first=rd32(r+4), rot_first=rd32(r+12), scl_first=rd32(r+16);
        uint16_t pos_count=rd16(r+8), rot_count=rd16(r+10), scl_count=rd16(r+20);
        if (!valid_range(pos_first,pos_count,keys->count) || !valid_range(rot_first,rot_count,keys->count) || !valid_range(scl_first,scl_count,keys->count)) return SA3C_ERR_REFERENCE;
    }
    return SA3C_OK;
}

const char *sa3c_result_name(SA3CResult r) {
    switch(r){
    case SA3C_OK:return "OK"; case SA3C_ERR_ARGUMENT:return "ARGUMENT"; case SA3C_ERR_TRUNCATED:return "TRUNCATED";
    case SA3C_ERR_MAGIC:return "MAGIC"; case SA3C_ERR_VERSION:return "VERSION"; case SA3C_ERR_HEADER:return "HEADER";
    case SA3C_ERR_SECTION:return "SECTION"; case SA3C_ERR_RANGE:return "RANGE"; case SA3C_ERR_REFERENCE:return "REFERENCE";
    default:return "UNKNOWN";
    }
}
