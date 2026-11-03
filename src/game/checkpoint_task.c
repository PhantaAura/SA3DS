#include "checkpoint_task.h"
#include <math.h>
#include <string.h>
static int32_t signed16(uint32_t n){return (n&0x8000u)?(int32_t)(n&65535u)-65536:(int32_t)(n&65535u);}
/*These source float multiplications have exact rational products within x87's
 * mantissa. Integer division reproduces truncation without native FPU drift.*/
static int32_t damp(int32_t n){return (int32_t)(((int64_t)n*8346665)/8388608);}
static void animate(SA3DSCheckpointTask*t,SA3DSCheckpointResult*out){
    t->post_x=signed16((uint32_t)t->post_x+(uint32_t)t->velocity_x);
    t->post_y=signed16((uint32_t)t->post_y+(uint32_t)t->velocity_y);
    t->velocity_x=signed16((uint32_t)t->velocity_x);
    t->velocity_y=damp(signed16((uint32_t)t->velocity_y));
    int32_t absolute=signed16((uint32_t)(t->velocity_y<0?-t->velocity_y:t->velocity_y));
    if(absolute<0xe38){
        int32_t target=t->post_x>0x2000?0x47d2-t->post_x:t->post_x;
        int64_t numerator=(int64_t)(0xe38-absolute)*signed16((uint32_t)target)*(-13421773);
        t->velocity_x=signed16((uint32_t)(numerator/INT64_C(1099511627776)));
    }
    if(!t->velocity_x&&!t->velocity_y){t->action=3;return;}
    if(t->effect_count<3){out->emit_effect=1;out->effect_identifier=0x38e;t->effect_count++;}
}
int sa3ds_checkpoint_spin(double speed,int16_t*out){
    if(!out||!isfinite(speed))return 0;
    double angular=speed*10.*65536.*0.0027777777777777778838;
    if(angular<-2147483648.||angular>=2147483648.)return 0;
    *out=(int16_t)signed16((uint32_t)(int32_t)trunc(angular));return 1;
}
int sa3ds_checkpoint_task_tick(SA3DSCheckpointTask*t,const SA3DSCheckpointInput*in,SA3DSCheckpointResult*out){
    if(!t||!in||!out)return 0;
    memset(out,0,sizeof(*out));if(in->clipped)return 1;
    if(t->action>4)return 0;
    if(t->action==0){
        out->initialize=1;t->action=1;t->post_x=0x4000;t->post_y=-0x4000;
        if(in->persisted){t->post_x=0;t->action=3;}
        return 1;
    }
    out->display=1;out->active_material=t->action!=1;
    out->draw_post_x=t->post_x;out->draw_post_y=t->post_y;
    if(t->action==4)return 1;
    out->sync_children=1;out->enqueue_collision=1;
    out->child_post_x=t->post_x;out->child_post_y=t->post_y;
    if(t->action==1&&in->prior_collision&&in->other_is_player0){
        t->action=2;t->velocity_y=in->activation_spin;t->time_display=300;
        out->capture_restart=in->player_present;out->persist_set=1;
        out->play_sound=1;out->sound_id=10;
    }else if(t->action==2&&in->player_present)animate(t,out);
    /*Only cases2/3 enter44F2D0, never the action1 activation frame.*/
    if(out->active_material&&t->time_display>0){
        t->time_display--;
        int phase=(300-t->time_display)/10;
        out->display_time=!(phase>=1&&phase<=11&&(phase&1));
    }
    return 1;
}
