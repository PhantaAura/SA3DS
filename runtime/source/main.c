#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physics.h"
#include "input_adapter.h"
#include "lab_package.h"
#include "sa3dspak.h"
#include "resource_metrics_3ds.h"

#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240
#define PI 3.14159265358979323846f
#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

typedef struct { float x,y,z; } V3;
typedef struct { int x,y; bool ok; } P2;

static SaPlayer player;
static float camera_yaw = 0.0f;
static bool dev_overlay = false;
static bool package_ok = false;
static SA3DSResourceMetrics resource_metrics;

static inline void putpx(u8 *fb, int w, int h, int x, int y, u8 r, u8 g, u8 b) {
    if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
    /* 3DS framebuffer is column-major BGR8. */
    int i = 3 * (y + x * h);
    fb[i+0] = b; fb[i+1] = g; fb[i+2] = r;
}

static void clearfb(u8 *fb, int w, int h, u8 r, u8 g, u8 b) {
    for (int x=0;x<w;x++) for (int y=0;y<h;y++) putpx(fb,w,h,x,y,r,g,b);
}

static void line(u8 *fb,int w,int h,int x0,int y0,int x1,int y1,u8 r,u8 g,u8 b) {
    int dx=abs(x1-x0), sx=x0<x1?1:-1;
    int dy=-abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;){
        putpx(fb,w,h,x0,y0,r,g,b);
        if(x0==x1 && y0==y1) break;
        int e2=2*err;
        if(e2>=dy){err+=dy;x0+=sx;}
        if(e2<=dx){err+=dx;y0+=sy;}
    }
}

static void rect(u8 *fb,int w,int h,int x,int y,int rw,int rh,u8 r,u8 g,u8 b){
    for(int ix=x;ix<x+rw;ix++) for(int iy=y;iy<y+rh;iy++) putpx(fb,w,h,ix,iy,r,g,b);
}

static P2 project(V3 world) {
    float cy=cosf(-camera_yaw), sy=sinf(-camera_yaw);
    float dx=world.x-player.pos.x;
    float dz=world.z-player.pos.z;
    float rx=dx*cy-dz*sy;
    float rz=dx*sy+dz*cy;
    float ry=world.y-(player.pos.y+5.0f);
    rz += 15.0f; /* chase-camera distance */
    P2 p={0,0,false};
    if(rz<1.0f) return p;
    float f=150.0f/rz;
    p.x=(int)(TOP_W*0.5f + rx*f);
    p.y=(int)(TOP_H*0.58f - ry*f);
    p.ok=(p.x>-100 && p.x<TOP_W+100 && p.y>-100 && p.y<TOP_H+100);
    return p;
}

static void draw_world_line(u8*fb,V3 a,V3 b,u8 r,u8 g,u8 bl){
    P2 pa=project(a), pb=project(b);
    if(pa.ok && pb.ok) line(fb,TOP_W,TOP_H,pa.x,pa.y,pb.x,pb.y,r,g,bl);
}

static void draw_course(u8 *fb) {
    /* White-space arena grid; all geometry is original/simple test geometry. */
    for(int z=-48; z<=240; z+=8){
        SaSurface l=sa_course_surface(-12.0f,(float)z);
        SaSurface r=sa_course_surface( 12.0f,(float)z);
        if(l.valid && r.valid){
            draw_world_line(fb,(V3){-12,l.ground_y,z},(V3){12,r.ground_y,z},180,198,218);
        }
    }
    for(int side=-1;side<=1;side+=2){
        float x=12.0f*side;
        V3 last={0}; bool have=false;
        for(int z=-48;z<=240;z+=3){
            SaSurface s=sa_course_surface(x,(float)z);
            if(s.valid){
                V3 cur={x,s.ground_y,(float)z};
                if(have) draw_world_line(fb,last,cur,82,121,164);
                last=cur;have=true;
            } else have=false;
        }
    }
    /* center guide */
    V3 last={0}; bool have=false;
    for(int z=-48;z<=240;z+=3){
        SaSurface s=sa_course_surface(0,(float)z);
        if(s.valid){
            V3 cur={0,s.ground_y+0.03f,(float)z};
            if(have) draw_world_line(fb,last,cur,100,150,205);
            last=cur;have=true;
        } else have=false;
    }

    /* 0.0.8.8 original loop/contact rig. Physics uses the arbitrary-contact
       callback; these rings are only a simple visual guide. */
    for(int xi=-10; xi<=10; xi+=10){
        V3 prev={0}; bool loop_have=false;
        for(int deg=0;deg<=360;deg+=10){
            float a=(float)deg*PI/180.0f;
            V3 cur={(float)xi,18.0f-14.0f*cosf(a),225.0f+14.0f*sinf(a)};
            if(loop_have) draw_world_line(fb,prev,cur,88,132,175);
            prev=cur;loop_have=true;
        }
    }
}

static void draw_player(u8 *fb) {
    P2 p=project((V3){player.pos.x,player.pos.y+1.1f,player.pos.z});
    if(!p.ok) return;
    rect(fb,TOP_W,TOP_H,p.x-5,p.y-8,10,14,38,105,220);
    rect(fb,TOP_W,TOP_H,p.x-6,p.y+6,5,3,220,55,55);
    rect(fb,TOP_W,TOP_H,p.x+1,p.y+6,5,3,220,55,55);
}

static void draw_top(u8 *fb) {
    clearfb(fb,TOP_W,TOP_H,245,249,253);
    /* horizon band */
    rect(fb,TOP_W,TOP_H,0,0,TOP_W,70,232,241,250);
    line(fb,TOP_W,TOP_H,0,70,TOP_W-1,70,191,211,231);
    draw_course(fb);
    draw_player(fb);

    if(dev_overlay){
        rect(fb,TOP_W,TOP_H,4,4,130,42,20,30,45);
        /* minimal graphical bars; text is on bottom console */
        int sp=(int)(CLAMP(sa_horizontal_speed(&player)/3.00f,0,1)*120);
        rect(fb,TOP_W,TOP_H,9,12,120,7,65,77,93);
        rect(fb,TOP_W,TOP_H,9,12,sp,7,85,164,245);
    }
}

int main(int argc,char**argv){
    (void)argc; (void)argv;
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);
    sa_physics_reset(&player);
    SA3DSPak lab_package;
    package_ok = sa3dspak_open(&lab_package, SA3DS_LAB_PACKAGE, SA3DS_LAB_PACKAGE_SIZE) == SA3DSPAK_OK;
    resource_metrics.package_bytes = SA3DS_LAB_PACKAGE_SIZE;
    printf("\x1b[2;2HSA3DS MOVEMENT LAB 0.0.8.8.9\n");
    printf("\x1b[4;2HCircle Pad  move/steer\n");
    printf("\x1b[5;2HA           jump / air action\n");
    printf("\x1b[6;2HB / X       contextual action\n");
    printf("\x1b[7;2HD-Down      Spin Dash shortcut\n");
    printf("\x1b[8;2HD-Up        Light Speed shortcut\n");
    printf("\x1b[9;2HD-Left/Right camera snap\n");
    printf("\x1b[10;2HL / R       rotate camera\n");
    printf("\x1b[11;2HY           camera recenter\n");
    printf("\x1b[12;2HSELECT      dev overlay\n");
    printf("\x1b[13;2HSTART       exit (dev sandbox)\n");
    printf("\x1b[15;2HSteam flat+air+ball + surface candidate.\n");
    printf("\x1b[16;2HNo Sega assets bundled.\n");

    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        u32 held=hidKeysHeld();
        u32 up=hidKeysUp();
        if(down&KEY_START) break;
        if(down&KEY_SELECT) dev_overlay=!dev_overlay;
        if(down&KEY_Y) camera_yaw=player.yaw;
        /* Restored camera utility: 45-degree presentation-only snap. */
        const float camera_snap = PI * 0.25f;
        if(down&KEY_DLEFT) camera_yaw -= camera_snap;
        if(down&KEY_DRIGHT) camera_yaw += camera_snap;
        /* Manual camera is a presentation control, separate from player physics. */
        const float camera_manual_step = 0.035f; /* provisional feel value */
        if(held&KEY_L) camera_yaw -= camera_manual_step;
        if(held&KEY_R) camera_yaw += camera_manual_step;

        SaInput in=sa3ds_read_logical_input(down,held,up);

        /* Fixed 60 Hz simulation: exactly one deterministic step per vblank. */
        sa_physics_step(&player,&in,camera_yaw);

        /* gentle chase orientation; Y snaps camera to player yaw */
        float d=player.yaw-camera_yaw;
        while(d>PI) d-=2*PI;
        while(d<-PI) d+=2*PI;
        if(!(held&(KEY_L|KEY_R))) camera_yaw += d*0.025f;

        u16 w,h;
        u8 *top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&w,&h);
        draw_top(top);

        if(dev_overlay){
            sa3ds_query_resource_metrics(&resource_metrics);
            printf("\x1b[18;2HSPD:%5.3f PK:%5.3f          ",sa_horizontal_speed(&player),player.peak_speed);
            printf("\x1b[19;2HPOS:%6.1f %5.1f %6.1f   ",player.pos.x,player.pos.y,player.pos.z);
            printf("\x1b[20;2H%s ST:%d F:%llu          ",player.grounded?"GROUND":"AIR   ",
                   (int)player.action_state,(unsigned long long)player.sim_frame);
            printf("\x1b[21;2HPAK:%s SID:%ld CH:%4.1f      ",package_ok?"OK":"BAD",
                   (long)player.surface_id,player.spindash_charge);
            printf("\x1b[22;2HLIN:%luKB PAK:%luB         ",
                   (unsigned long)(resource_metrics.linear_free/1024u),
                   (unsigned long)resource_metrics.package_bytes);
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
