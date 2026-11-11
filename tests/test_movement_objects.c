#include "movement_objects.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
    assert(argc>=2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);
    void*bytes=malloc((size_t)size);assert(bytes);assert(fread(bytes,1,(size_t)size,f)==(size_t)size);fclose(f);
    SA3DSMovementObjects o={0};int ok=sa3ds_movement_objects_open(&o,bytes,(size_t)size);
    if(argc>2&&!strcmp(argv[2],"extended")){
        SA3DSMovementPlacement p;
        assert(ok&&o.count==12);
        assert(sa3ds_movement_objects_placement(&o,3,&p)&&p.source_row==4&&p.type==40);
        assert(sa3ds_movement_objects_placement(&o,4,&p)&&p.source_row==5&&p.type==18);
        assert(sa3ds_movement_objects_placement(&o,5,&p)&&p.source_row==6&&p.type==26);
        free(bytes);return 0;
    }
    if(argc>2&&!strcmp(argv[2],"owner")){
        assert(ok);unsigned counts[2][41]={{0}};unsigned springs=0, pier_exit=0, tikal_hint=0;
        for(uint32_t i=0;i<o.count;i++){
            SA3DSMovementPlacement p;SA3DSSpringCommand c;
            assert(sa3ds_movement_objects_placement(&o,i,&p));
            unsigned kind=p.type&4095;
            counts[p.act][kind]++;
            if(p.act==0&&p.source_row==537&&kind==40&&p.parameters[0]==11.f&&p.parameters[1]==60.f)
                pier_exit++;
            if(p.act==1&&p.source_row==694&&kind==26&&p.parameters[2]==8.f)
                tikal_hint++;
            if(kind==1||kind==2){assert(sa3ds_movement_spring_command(&p,&c));assert(c.strength==p.parameters[1]+5.f);springs++;}
        }
        assert(counts[0][1]==0&&counts[0][2]==11&&counts[0][3]==28);
        assert(counts[1][1]==5&&counts[1][2]==10&&counts[1][3]==21&&springs==26);
        assert(counts[0][40]==4&&counts[1][40]==2);
        assert(counts[0][18]==0&&counts[1][18]==6);
        assert(counts[0][26]==0&&counts[1][26]==1&&pier_exit==1&&tikal_hint==1);
        free(bytes);puts("Exact-owner movement definitions: PASS (88 placements, 26 spring commands)");return 0;
    }
    if(argc>2){assert(!ok&&!o.count&&!o.rows);free(bytes);return 0;}
    assert(ok&&o.count==6);SA3DSMovementPlacement p;SA3DSSpringCommand c;
    assert(sa3ds_movement_objects_placement(&o,0,&p));
    assert(p.act==0&&p.source_row==1&&p.type==0x8002&&p.angles[0]==-16384&&p.angles[2]==-3);
    assert(p.position.x==100&&p.parameters[2]==13);
    assert(sa3ds_movement_spring_command(&p,&c));
    assert(c.strength==7.25f&&c.control_timer==93&&c.command==4&&c.sonic_action==10&&c.animation_request==16);
    p.parameters[0]=-12.5f;assert(sa3ds_movement_spring_command(&p,&c)&&c.control_timer==0);
    p.parameters[0]=32768.75f;assert(sa3ds_movement_spring_command(&p,&c)&&c.control_timer==-32768);
    p.parameters[0]=65537.75f;assert(sa3ds_movement_spring_command(&p,&c)&&c.control_timer==1);
    p.parameters[0]=2147483648.f;assert(!sa3ds_movement_spring_command(&p,&c)&&c.strength==0);
    p.parameters[0]=NAN;assert(!sa3ds_movement_spring_command(&p,&c));
    assert(sa3ds_movement_objects_placement(&o,2,&p)&&p.type==1);
    p.parameters[0]=180;assert(sa3ds_movement_spring_command(&p,&c));
    assert(c.control_timer==0&&c.command==3&&c.sonic_action==9&&c.strength==7.25f);
    p.parameters[1]=INFINITY;assert(!sa3ds_movement_spring_command(&p,&c));
    assert(sa3ds_movement_objects_placement(&o,1,&p)&&p.type==3&&!sa3ds_movement_spring_command(&p,&c));
    assert(!sa3ds_movement_objects_placement(&o,6,&p));
    assert(!sa3ds_movement_objects_open(&o,bytes,16)&&!o.rows&&!o.count);
    free(bytes);puts("Movement object reader/command decoding: PASS (not gameplay integration)");return 0;
}
