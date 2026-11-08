#include "cam_select.h"
#include <string.h>
typedef struct {
 SA3DSCamSelect *s;const SA3DSCamTables *t;const SA3DSCamRecord *r;uint32_t n;uint8_t hb;
 const float *cur,*h1,*h2;double(*sine)(void*,uint16_t);void *ctx;int ok;
} Ctx;
static int8_t sub(const Ctx*c,int16_t record){ /*record+1 as signed char*/
 if(record<0||(uint32_t)record>=c->n)return (int8_t)c->hb;
 return (int8_t)c->r[record].priority;
}
static uint8_t rec_adjust(const Ctx*c,int16_t record){return record<0||(uint32_t)record>=c->n?0x15:c->r[record].adjust;}
static int8_t level_of(const SA3DSCamTables*t,uint8_t mode){return (int8_t)t->priority[mode];}
static int valid_level(int8_t l){return l>=0&&(unsigned)l<SA3DS_CAM_LEVELS;}
static int inside(Ctx*c,const SA3DSCamRecord*r){
 int v=0;if(!sa3ds_cam_trigger_inside(r,c->cur,c->h1,c->h2,c->sine,c->ctx,&v)){c->ok=0;return 0;}return v;
}
/*435DE0*/
static void enter(Ctx*c,int16_t di){
 SA3DSCamSelect*s=c->s;const SA3DSCamRecord*rec=&c->r[di];int8_t lvl=level_of(c->t,rec->mode);
 if(!valid_level(lvl)){c->ok=0;return;}
 int8_t old_sub=sub(c,s->record),new_sub=(int8_t)rec->priority;
 s->saved[lvl]=(SA3DSCamSaved){s->priority,s->record,s->flags,s->mode,rec_adjust(c,s->record)};
 s->priority=lvl;s->record=di;s->mode=rec->mode;
 if(old_sub<new_sub)s->subpriority_rose=1;
 s->adjust=(old_sub<10||new_sub<10)?rec->adjust:0;
 s->reset_adjust=1;
 s->flags=c->t->adjust_flag[s->adjust]?(uint8_t)(s->flags|1):0;
 s->apply=c->t->apply[s->mode];s->changed=1;
}
/*435F50*/
static void leave(Ctx*c){
 SA3DSCamSelect*s=c->s;int8_t lvl=s->priority;if(!valid_level(lvl)){c->ok=0;return;}
 SA3DSCamSaved v=s->saved[lvl];int8_t c1=sub(c,s->record),c2=sub(c,v.record);
 s->record=v.record;s->flags=v.flags;s->mode=v.mode;s->priority=v.priority;
 s->adjust=(c1<10||c2<10)?v.adjust:0; /*cae8 follows: saved function or adjust table[0]*/
 if(c1<c2)s->subpriority_rose=1;
 s->changed=1;s->reset_frames=1;s->reset_adjust=1;s->apply=c->t->apply[v.mode];
}
static void set_mode(SA3DSCamSelect*s,const SA3DSCamTables*t,const SA3DSCamRecord*records,uint32_t n,uint8_t mode,uint8_t adjust){
 if(s->priority!=0)return;                                   /*436040 guard*/
 int16_t r=s->record;
 s->saved[0]=(SA3DSCamSaved){0,r,s->flags,s->mode,(uint8_t)(r<0||(uint32_t)r>=n||!records?0x15:records[r].adjust)};
 s->record=-1;s->priority=0;s->mode=mode;s->adjust=adjust;s->reset_adjust=1;
 if(t->adjust_flag[adjust])s->flags|=1;
 s->apply=t->apply[mode];s->changed=1;
}
/*436140*/
static void pop2(Ctx*c){
 SA3DSCamSelect*s=c->s;if(s->priority!=2)return;
 SA3DSCamSaved v=s->saved[2];
 s->record=v.record;s->flags=v.flags;s->mode=v.mode;s->adjust=v.adjust;s->priority=v.priority;
 s->apply=c->t->apply[v.mode];s->changed=1;s->reset_frames=1;s->reset_adjust=1;
 if(v.priority==0&&s->default_mode!=-1)set_mode(s,c->t,c->r,c->n,(uint8_t)s->default_mode,s->default_adjust);
}
void sa3ds_cam_select_init(SA3DSCamSelect*s,const SA3DSCamTables*t,uint8_t mode,uint8_t adjust){
 if(!s||!t)return;memset(s,0,sizeof *s);s->record=-1;s->default_mode=-1;
 set_mode(s,t,NULL,0,mode,adjust);
}
void sa3ds_cam_select_set(SA3DSCamSelect*s,const SA3DSCamTables*t,const SA3DSCamRecord*records,uint32_t count,uint8_t mode,uint8_t adjust){
 if(s&&t)set_mode(s,t,records,count,mode,adjust);
}
int sa3ds_cam_select_update(SA3DSCamSelect*s,const SA3DSCamTables*t,const SA3DSCamRecord*records,uint32_t count,
 uint8_t hb,const float cur[3],const float h1[3],const float h2[3],double(*sine)(void*,uint16_t),void*ctx){
 if(!s||!t||!t->priority||!t->apply||!t->adjust_flag||!cur||!h1||!h2||!sine||(count&&!records)||count>32767u)return 0;
 if(s->mode==64||s->mode==76)return 1;                        /*idle/event camera holds*/
 Ctx c={s,t,records,count,hb,cur,h1,h2,sine,ctx,1};
 int8_t p=s->priority;
restart:;
 int8_t start=p;
 for(int32_t i=(int32_t)count-1;i>=1;i--){
  const SA3DSCamRecord*rec=&records[i];int8_t rp=level_of(t,rec->mode);int16_t di=(int16_t)i;
  if(rp<p)continue;
  switch(p){
  case 0: /*437750*/
   if(rp!=4&&s->record!=di&&inside(&c,rec))enter(&c,di);
   break;
  case 1:case 3: /*4377A0*/
   if(rp==4)break;
   if(s->record==di){
    if(rec->shape==0||rec->shape==2){if(!inside(&c,rec))leave(&c);}
    else leave(&c);                                            /*gate record leaves at once*/
   }else if(rp>p){
    if((rec->shape==0||rec->shape==2)&&inside(&c,rec))enter(&c,di);
   }else if(sub(&c,s->record)<(int8_t)rec->priority&&(rec->shape==0||rec->shape==2)&&inside(&c,rec)){
    leave(&c);enter(&c,di);
   }
   break;
  case 2: /*437900*/
   if(rp==4){
    if(rec->shape==1&&inside(&c,rec)){
     unsigned slot=2;                                          /*walk to the level that returns to 0*/
     if(s->saved[2].priority!=0){
      slot=2;for(unsigned guard=0;guard<SA3DS_CAM_LEVELS;guard++){
       int8_t next=s->saved[slot].priority;if(!valid_level(next)){c.ok=0;break;}
       slot=(unsigned)next;if(s->saved[slot].priority==0)break;
      }
     }
     if(rec->mode!=61)s->saved[slot].mode=rec->mode;
     s->saved[slot].adjust=rec->adjust;
    }
   }else if(sub(&c,s->record)<(int8_t)rec->priority&&(rec->shape==0||rec->shape==2)&&inside(&c,rec)){
    pop2(&c);enter(&c,di);
   }
   break;
  default:break;
  }
  if(!c.ok)return 0;
  p=s->priority;
  if(p<start)goto restart;
 }
 return c.ok;
}
