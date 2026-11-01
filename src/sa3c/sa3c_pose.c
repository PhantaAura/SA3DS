#include "sa3c_pose.h"

#include <math.h>
#include <string.h>

#define SA3C_INVALID_U16 0xFFFFu
#define SA3C_BAMS_TO_RAD (6.28318530717958647692f / 65536.0f)
#define SA3C_RAD_TO_BAMS (65536.0f / 6.28318530717958647692f)
#define SA3C_MTYPE_FN_MASK 0x00C0u
#define SA3C_MTYPE_LINEAR  0x0000u

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static float rdf(const uint8_t *p) {
    uint32_t u = rd32(p); float f; memcpy(&f, &u, sizeof(f)); return f;
}
static const uint8_t *section_data(const SA3CCharacter *c, unsigned index) {
    return c->data + c->sections[index].offset;
}

void sa3c_matrix_identity(SA3CMatrix *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->m[0] = out->m[5] = out->m[10] = out->m[15] = 1.0f;
}

/* These mutate the matrix in the same element pattern/order as SA Tools'
 * MatrixFunctions.cs, which mirrors Ninja object evaluation. */
static void nj_translate(SA3CMatrix *a, float x, float y, float z) {
    float *m=a->m;
    m[12] = z*m[8]  + y*m[4]  + x*m[0] + m[12];
    m[13] = z*m[9]  + y*m[5]  + x*m[1] + m[13];
    m[14] = z*m[10] + y*m[6]  + x*m[2] + m[14];
    m[15] = z*m[11] + y*m[7]  + x*m[3] + m[15];
}
static void nj_rotx(SA3CMatrix *a,float x) {
    float *m=a->m, s=sinf(x), c=cosf(x);
    float m21=m[4],m22=m[5],m23=m[6],m24=m[7];
    m[4]=s*m[8]+m21*c; m[5]=s*m[9]+m22*c; m[6]=m23*c+s*m[10]; m[7]=s*m[11]+m24*c;
    m[8]=c*m[8]-m21*s; m[9]=c*m[9]-m22*s; m[10]=c*m[10]-m23*s; m[11]=c*m[11]-m24*s;
}
static void nj_roty(SA3CMatrix *a,float y) {
    float *m=a->m, s=sinf(y), c=cosf(y);
    float m11=m[0],m12=m[1],m13=m[2],m14=m[3];
    m[0]=m[0]*c-s*m[8]; m[1]=m12*c-s*m[9]; m[2]=m13*c-s*m[10]; m[3]=m14*c-s*m[11];
    m[8]=c*m[8]+m11*s; m[9]=m12*s+c*m[9]; m[10]=c*m[10]+m13*s; m[11]=m14*s+c*m[11];
}
static void nj_rotz(SA3CMatrix *a,float z) {
    float *m=a->m, s=sinf(z), c=cosf(z);
    float m11=m[0],m12=m[1],m13=m[2],m14=m[3];
    m[0]=s*m[4]+m11*c; m[1]=s*m[5]+m12*c; m[2]=s*m[6]+m13*c; m[3]=s*m[7]+m14*c;
    m[4]=c*m[4]-m11*s; m[5]=c*m[5]-m12*s; m[6]=c*m[6]-m13*s; m[7]=c*m[7]-m14*s;
}
static void nj_rotate_xyz(SA3CMatrix *a, SA3CVec3 r) {
    /* SA Tools RotateXYZ invokes Z, then Y, then X. */
    if (r.z != 0.0f) nj_rotz(a,r.z);
    if (r.y != 0.0f) nj_roty(a,r.y);
    if (r.x != 0.0f) nj_rotx(a,r.x);
}
static void nj_rotate_zyx_flag(SA3CMatrix *a, SA3CVec3 r) {
    /* SA Tools RotateZYX invokes Y, then X, then Z. */
    if (r.y != 0.0f) nj_roty(a,r.y);
    if (r.x != 0.0f) nj_rotx(a,r.x);
    if (r.z != 0.0f) nj_rotz(a,r.z);
}
static void nj_scale(SA3CMatrix *a,float x,float y,float z) {
    float *m=a->m;
    for (int i=0;i<4;i++) m[i]*=x;
    for (int i=4;i<8;i++) m[i]*=y;
    for (int i=8;i<12;i++) m[i]*=z;
}

void sa3c_matrix_build_object(SA3CMatrix *out, SA3CVec3 position,
    SA3CVec3 rotation, SA3CVec3 scale, uint32_t eval_flags) {
    if (!out) return;
    sa3c_matrix_identity(out);
    if (!(eval_flags&SA3C_EVAL_UNIT_POS))
        nj_translate(out,position.x,position.y,position.z);
    if (!(eval_flags&SA3C_EVAL_UNIT_ANG)) {
        if (eval_flags&SA3C_EVAL_ZXY_ANG) nj_rotate_zyx_flag(out,rotation);
        else nj_rotate_xyz(out,rotation);
    }
    if (!(eval_flags&SA3C_EVAL_UNIT_SCL))
        nj_scale(out,scale.x,scale.y,scale.z);
}

SA3CPoseResult sa3c_pose_bind(const SA3CCharacter *c, SA3CTransform *local, size_t local_count) {
    if (!c || !c->data || !local) return SA3C_POSE_ERR_ARGUMENT;
    const SA3CSection *node=&c->sections[0];
    if (local_count < node->count) return SA3C_POSE_ERR_RANGE;
    const uint8_t *nodes=section_data(c,0);
    for (uint32_t i=0;i<node->count;i++) {
        const uint8_t *r=nodes+i*node->stride;
        local[i].position=(SA3CVec3){rdf(r+12),rdf(r+16),rdf(r+20)};
        local[i].rotation=(SA3CVec3){rdf(r+24),rdf(r+28),rdf(r+32)};
        local[i].scale=(SA3CVec3){rdf(r+36),rdf(r+40),rdf(r+44)};
    }
    return SA3C_POSE_OK;
}

static float normalize_frame(float frame,uint16_t frames,int loop) {
    if (frames==0) return 0.0f;
    if (loop) {
        frame=fmodf(frame,(float)frames);
        if (frame<0.0f) frame+=(float)frames;
        return frame;
    }
    if (frame<0.0f) return 0.0f;
    if (frame>(float)(frames-1u)) return (float)(frames-1u);
    return frame;
}

static void key_vec(const uint8_t *key, SA3CVec3 *v) {
    *v=(SA3CVec3){rdf(key+4),rdf(key+8),rdf(key+12)};
}
static uint32_t key_frame(const uint8_t *key){ return rd32(key); }

static SA3CVec3 sample_vec_linear(const uint8_t *keys,uint32_t first,uint16_t count,uint16_t frames,float frame) {
    const uint8_t *base=keys+(size_t)first*16u;
    SA3CVec3 a={0,0,0},b={0,0,0};
    if (count==0) return a;
    if (count==1) { key_vec(base,&a); return a; }
    int prev=-1,next=-1;
    for (uint16_t i=0;i<count;i++) {
        uint32_t f=key_frame(base+(size_t)i*16u);
        if ((float)f==frame) { key_vec(base+(size_t)i*16u,&a); return a; }
        if ((float)f<frame) prev=(int)i;
        if ((float)f>frame) { next=(int)i; break; }
    }
    float af,bf;
    if (prev<0) { prev=(int)count-1; af=(float)key_frame(base+(size_t)prev*16u)-(float)frames; }
    else af=(float)key_frame(base+(size_t)prev*16u);
    if (next<0) { next=0; bf=(float)key_frame(base)+(float)frames; }
    else bf=(float)key_frame(base+(size_t)next*16u);
    key_vec(base+(size_t)prev*16u,&a); key_vec(base+(size_t)next*16u,&b);
    float den=bf-af; float t=den!=0.0f?(frame-af)/den:0.0f;
    return (SA3CVec3){a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
}

static float bams_lerp_component(float arad,float brad,double t) {
    /* PC2004 404C10 loads signed integer keys and retains a*(1-t)+b*t
     * before 644340 truncates toward zero. Rounding only recovers original
     * integer keys from the package's float radians; it is not interpolation.
     * double preserves the ratio beyond f32, without claiming x87 trace parity. */
    double a=roundf(arad*SA3C_RAD_TO_BAMS);
    double b=roundf(brad*SA3C_RAD_TO_BAMS);
    /* Separate product stores prevent a host FMA from changing an integer
     * truncation boundary. Original x87 retained80 precision is not emulated. */
    volatile double left=a*(1.0-t),right=b*t;
    return (float)trunc(left+right)*SA3C_BAMS_TO_RAD;
}
static SA3CVec3 sample_rot_linear(const uint8_t *keys,uint32_t first,uint16_t count,uint16_t frames,float frame) {
    (void)frames;
    const uint8_t *base=keys+(size_t)first*16u;
    SA3CVec3 a={0,0,0},b={0,0,0};
    if (count==0) return a;
    if (count==1) { key_vec(base,&a); return a; }
    /* 404B20 selects the last key <= trunc(frame), with initial index0.
     * 404C10 wraps the next index, but uses denominator1 for nonpositive
     * signed time differences. Do not invent a shortest-angle path. */
    uint16_t prev=0;
    uint32_t integer_frame=(uint32_t)frame;
    for(uint16_t i=1;i<count;i++){
        if(key_frame(base+(size_t)i*16u)>integer_frame)break;
        prev=i;
    }
    uint16_t next=(uint16_t)((prev+1u)%count);
    uint32_t af=key_frame(base+(size_t)prev*16u),bf=key_frame(base+(size_t)next*16u);
    key_vec(base+(size_t)prev*16u,&a);key_vec(base+(size_t)next*16u,&b);
    if((double)frame==af)return a;
    double denominator=bf>af?(double)(float)(bf-af):1.0;
    double t=((double)frame-(double)af)/denominator;
    return (SA3CVec3){bams_lerp_component(a.x,b.x,t),bams_lerp_component(a.y,b.y,t),bams_lerp_component(a.z,b.z,t)};
}

SA3CPoseResult sa3c_pose_sample_action(const SA3CCharacter *c,uint16_t action_index,float frame,int loop,SA3CTransform *local,size_t local_count,uint16_t *out_root_node) {
    if (!c || !c->data || !local || !isfinite(frame)) return SA3C_POSE_ERR_ARGUMENT;
    if ((uint32_t)action_index>=c->sections[7].count) return SA3C_POSE_ERR_RANGE;
    SA3CPoseResult pr=sa3c_pose_bind(c,local,local_count); if(pr!=SA3C_POSE_OK)return pr;
    const uint8_t *anims=section_data(c,7), *chans=section_data(c,8), *keys=section_data(c,9);
    const uint8_t *a=anims+(size_t)action_index*c->sections[7].stride;
    uint16_t root=rd16(a+2), frames=rd16(a+4), motion_word=rd16(a+6);
    if(root==SA3C_INVALID_U16 || frames==0) return SA3C_POSE_ERR_EMPTY_ACTION;
    if((motion_word&SA3C_MTYPE_FN_MASK)!=SA3C_MTYPE_LINEAR) return SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION;
    frame=normalize_frame(frame,frames,loop);
    uint32_t first=rd32(a+8),count=rd32(a+12);
    for(uint32_t i=0;i<count;i++) {
        const uint8_t *ch=chans+(size_t)(first+i)*c->sections[8].stride;
        uint16_t node=rd16(ch), flags=rd16(ch+2), pc=rd16(ch+8), rc=rd16(ch+10), sc=rd16(ch+20);
        if((uint32_t)node>=c->sections[0].count) return SA3C_POSE_ERR_RANGE;
        if(flags&1u) local[node].position=sample_vec_linear(keys,rd32(ch+4),pc,frames,frame);
        if(flags&2u) local[node].rotation=sample_rot_linear(keys,rd32(ch+12),rc,frames,frame);
        if(flags&4u) local[node].scale=sample_vec_linear(keys,rd32(ch+16),sc,frames,frame);
    }
    if(out_root_node)*out_root_node=root;
    return SA3C_POSE_OK;
}

static float transition_lerp(float a,float b,float t) {
    return a+(b-a)*t;
}

static float transition_bams_lerp(float arad,float brad,float t) {
    /* 43F6CC normalizes both synthetic keys to signed16; 4383D0 unwraps
     * destination along the signed16 delta (half-turn tie is negative).
     * The resulting two-key motion is drawn by ordinary 404C10. */
    int32_t a=(int16_t)(int32_t)roundf(arad*SA3C_RAD_TO_BAMS);
    int32_t b=(int16_t)(int32_t)roundf(brad*SA3C_RAD_TO_BAMS);
    int16_t delta=(int16_t)(b-a);
    volatile double left=(double)a*(1.0-(double)t),right=(double)(a+delta)*(double)t;
    return (float)trunc(left+right)*SA3C_BAMS_TO_RAD;
}

static float capture_bams(float arad,float brad,float ratio) {
    int32_t a=(int32_t)roundf(arad*SA3C_RAD_TO_BAMS);
    int32_t b=(int32_t)roundf(brad*SA3C_RAD_TO_BAMS);
    /* 43F612 stores the ratio as f32; 43F627 truncates the increment
     * before adding it to the original key, unlike ordinary sampling. */
    int32_t captured=a+(int32_t)trunc((double)(int16_t)(b-a)*(double)ratio);
    return (float)(int16_t)captured*SA3C_BAMS_TO_RAD;
}

static SA3CVec3 sample_rot_capture(const uint8_t *keys,uint32_t first,
    uint16_t count,uint16_t frames,float frame) {
    const uint8_t *base=keys+(size_t)first*16u;
    SA3CVec3 a={0,0,0},b={0,0,0};
    if(!count)return a;
    uint16_t prev=0,next=0;
    /* 43F54C..59F: retain the predecessor of the first key >= frame.
     * At the first key the source simply copies it. */
    if((double)key_frame(base)<(double)frame){
        for(next=1;next<count;next++){
            prev=(uint16_t)(next-1);
            if((double)key_frame(base+(size_t)next*16u)>=(double)frame)break;
        }
        if(next==count){prev=(uint16_t)(count-1);next=0;}
    }
    key_vec(base+(size_t)prev*16u,&a);key_vec(base+(size_t)next*16u,&b);
    float ratio=0;
    if(prev!=next){
        uint32_t af=key_frame(base+(size_t)prev*16u),bf=key_frame(base+(size_t)next*16u);
        double denominator=(double)bf-(double)af+(next<prev?(double)frames:0.0);
        if(denominator!=0.0)ratio=(float)(((double)frame-(double)af)/denominator);
    }
    return (SA3CVec3){capture_bams(a.x,b.x,ratio),capture_bams(a.y,b.y,ratio),capture_bams(a.z,b.z,ratio)};
}

static void capture_source_rotations(const SA3CCharacter*c,uint16_t action,
    float frame,SA3CTransform *local) {
    const uint8_t*a=section_data(c,7)+(size_t)action*c->sections[7].stride;
    uint16_t frames=rd16(a+4);
    frame=normalize_frame(frame,frames,1);
    uint32_t first=rd32(a+8),count=rd32(a+12);
    for(uint32_t i=0;i<count;i++){
        const uint8_t*ch=section_data(c,8)+(size_t)(first+i)*c->sections[8].stride;
        if(rd16(ch+2)&2u)local[rd16(ch)].rotation=sample_rot_capture(section_data(c,9),
            rd32(ch+12),rd16(ch+10),frames,frame);
    }
}

SA3CPoseResult sa3c_pose_sample_transition(const SA3CCharacter *c,
    uint16_t source_action_index,float source_frame,
    uint16_t destination_action_index,float weight,
    SA3CTransform *local,SA3CTransform *scratch,size_t local_count,
    uint16_t *out_root_node) {
    if(!c||!local||!scratch||!isfinite(weight)||weight<0.0f||weight>1.0f)
        return SA3C_POSE_ERR_ARGUMENT;
    uint16_t source_root=SA3C_INVALID_U16,destination_root=SA3C_INVALID_U16;
    SA3CPoseResult pr=sa3c_pose_sample_action(c,source_action_index,
        source_frame,1,scratch,local_count,&source_root);
    if(pr!=SA3C_POSE_OK)return pr;
    pr=sa3c_pose_sample_action(c,destination_action_index,0.0f,1,
        local,local_count,&destination_root);
    if(pr!=SA3C_POSE_OK)return pr;
    if(source_root!=destination_root)return SA3C_POSE_ERR_HIERARCHY;
    if(local_count<c->sections[0].count)return SA3C_POSE_ERR_RANGE;
    capture_source_rotations(c,source_action_index,source_frame,scratch);
    for(uint32_t i=0;i<c->sections[0].count;i++) {
        local[i].position.x=transition_lerp(scratch[i].position.x,local[i].position.x,weight);
        local[i].position.y=transition_lerp(scratch[i].position.y,local[i].position.y,weight);
        local[i].position.z=transition_lerp(scratch[i].position.z,local[i].position.z,weight);
        local[i].rotation.x=transition_bams_lerp(scratch[i].rotation.x,local[i].rotation.x,weight);
        local[i].rotation.y=transition_bams_lerp(scratch[i].rotation.y,local[i].rotation.y,weight);
        local[i].rotation.z=transition_bams_lerp(scratch[i].rotation.z,local[i].rotation.z,weight);
        local[i].scale.x=transition_lerp(scratch[i].scale.x,local[i].scale.x,weight);
        local[i].scale.y=transition_lerp(scratch[i].scale.y,local[i].scale.y,weight);
        local[i].scale.z=transition_lerp(scratch[i].scale.z,local[i].scale.z,weight);
    }
    if(out_root_node)*out_root_node=destination_root;
    return SA3C_POSE_OK;
}

static SA3CPoseResult walk_world(const SA3CCharacter *c,uint16_t node_index,const SA3CTransform *local,SA3CMatrix *world,const SA3CMatrix *parent,uint32_t depth,int animated) {
    if(node_index==SA3C_INVALID_U16)return SA3C_POSE_OK;
    if((uint32_t)node_index>=c->sections[0].count || depth>c->sections[0].count)return SA3C_POSE_ERR_HIERARCHY;
    const uint8_t *r=section_data(c,0)+(size_t)node_index*c->sections[0].stride;
    uint16_t child=rd16(r+2), sibling=rd16(r+4); uint32_t flags=rd32(r+8);
    const SA3CTransform *transform=&local[node_index];
    SA3CTransform bind;
    if(animated&&(flags&SA3C_EVAL_SKIP)){
        /* 4050DB bypasses motion callbacks and applies the node's bind TRS,
         * including components marked UNIT by the static evaluator. */
        bind.position=(SA3CVec3){rdf(r+12),rdf(r+16),rdf(r+20)};
        bind.rotation=(SA3CVec3){rdf(r+24),rdf(r+28),rdf(r+32)};
        bind.scale=(SA3CVec3){rdf(r+36),rdf(r+40),rdf(r+44)};
        transform=&bind;
    }
    SA3CMatrix m=*parent;
    if(animated||!(flags&SA3C_EVAL_UNIT_POS)) nj_translate(&m,transform->position.x,transform->position.y,transform->position.z);
    if(animated||!(flags&SA3C_EVAL_UNIT_ANG)) {
        if(flags&SA3C_EVAL_ZXY_ANG) nj_rotate_zyx_flag(&m,transform->rotation);
        else nj_rotate_xyz(&m,transform->rotation);
    }
    if(animated||!(flags&SA3C_EVAL_UNIT_SCL)) nj_scale(&m,transform->scale.x,transform->scale.y,transform->scale.z);
    world[node_index]=m;
    if((animated||!(flags&SA3C_EVAL_BREAK)) && child!=SA3C_INVALID_U16) {
        SA3CPoseResult pr=walk_world(c,child,local,world,&m,depth+1,animated); if(pr!=SA3C_POSE_OK)return pr;
    }
    if(sibling!=SA3C_INVALID_U16) return walk_world(c,sibling,local,world,parent,depth+1,animated);
    return SA3C_POSE_OK;
}

SA3CPoseResult sa3c_pose_build_world(const SA3CCharacter *c,uint16_t root_node,const SA3CTransform *local,size_t local_count,SA3CMatrix *world,size_t world_count) {
    if(!c||!c->data||!local||!world)return SA3C_POSE_ERR_ARGUMENT;
    if(local_count<c->sections[0].count||world_count<c->sections[0].count||(uint32_t)root_node>=c->sections[0].count)return SA3C_POSE_ERR_RANGE;
    SA3CMatrix identity; sa3c_matrix_identity(&identity);
    return walk_world(c,root_node,local,world,&identity,0,0);
}

SA3CPoseResult sa3c_pose_build_world_animated(const SA3CCharacter *c,uint16_t root_node,const SA3CTransform *local,size_t local_count,SA3CMatrix *world,size_t world_count) {
    if(!c||!c->data||!local||!world)return SA3C_POSE_ERR_ARGUMENT;
    if(local_count<c->sections[0].count||world_count<c->sections[0].count||(uint32_t)root_node>=c->sections[0].count)return SA3C_POSE_ERR_RANGE;
    SA3CMatrix identity;sa3c_matrix_identity(&identity);
    return walk_world(c,root_node,local,world,&identity,0,1);
}

const char *sa3c_pose_result_name(SA3CPoseResult r) {
    switch(r){
    case SA3C_POSE_OK:return "OK"; case SA3C_POSE_ERR_ARGUMENT:return "ARGUMENT"; case SA3C_POSE_ERR_RANGE:return "RANGE";
    case SA3C_POSE_ERR_EMPTY_ACTION:return "EMPTY_ACTION"; case SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION:return "UNSUPPORTED_INTERPOLATION";
    case SA3C_POSE_ERR_HIERARCHY:return "HIERARCHY"; default:return "UNKNOWN";
    }
}
