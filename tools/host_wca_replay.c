/* Owner-side native replay oracle. Reads existing external collision geometry;
 * emits controller traces, not game assets. Not an independent retail oracle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wca_replay.h"
#include "player_view.h"
#include "emerald_coast_collision.h"
#include "movement.h"
#include "steam_flat.h"
#include "parity_trace.h"

int main(int argc,char **argv) {
    if(argc!=3){fprintf(stderr,"usage: %s owner_collision.sa3dspak output.csv\n",argv[0]);return 2;}
    FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)){fclose(file);return 2;}
    long size=ftell(file);rewind(file);if(size<=0){fclose(file);return 2;}
    unsigned char *bytes=malloc((size_t)size);if(!bytes){fclose(file);return 2;}
    int read_ok=fread(bytes,1,(size_t)size,file)==(size_t)size;fclose(file);
    SA3DSECCollision collision;
    if(!read_ok || !sa3ds_ec_collision_open(&collision,bytes,(size_t)size)){free(bytes);return 1;}
    if(!sa3ds_ec_collision_spawn(&collision,0,SA3DS_EC_SPAWN_Z,0)){
        sa3ds_ec_collision_destroy(&collision);free(bytes);return 1;
    }
    FILE *output=fopen(argv[2],"wb");if(!output){sa3ds_ec_collision_destroy(&collision);free(bytes);return 2;}
    int ok=sa3ds_parity_trace_write_header(output)==0;
    SA1PlayerState player={0};SA3DSSonicAnimationRuntime runtime={0};SA3DSSonicAnimationSync sync={0};
    for(unsigned tick=0;ok && tick<SA3DS_WCA_REPLAY_TICKS;tick++) {
        SA3DSWCAReplayStep step=sa3ds_wca_replay_step(tick);
        if(step.reset) {
            sa1_player_reset(&player,&collision.world);
            collision.reference_y=player.pos.y;
            memset(&runtime,0,sizeof runtime);memset(&sync,0,sizeof sync);
            SA3DSSonicAnimationObservation observation=sa3ds_sonic_animation_observe(
                &sync,&player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
            (void)sa3ds_sonic_animation_select(&runtime,&player,&observation);
        }
        SA1Input input=sa3ds_view_input(step.screen_input);
        SA1Vec3 before=player.pos;uint64_t previous_tick=player.sim_frame;
        collision.reference_y=player.pos.y;
        SA1SteamFlatDebug debug={0};
        sa1_physics_step_steam_sonic_flat(&player,&input,step.camera_yaw,
            &collision.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&debug);
        if(!(previous_tick && player.sim_frame==1))sa3ds_ec_collision_resolve_walls(&collision,before,&player);
        SA1PlayerState physics_done=player;
        SA3DSSonicAnimationObservation observation=sa3ds_sonic_animation_observe(
            &sync,&player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
        SA3DSSonicAnimationSelection selection=sa3ds_sonic_animation_select(&runtime,&player,&observation);
        if(memcmp(&physics_done,&player,sizeof player)){ok=0;break;}
        char name[64];snprintf(name,sizeof name,"WCA_%03u_%s",step.case_id,step.name);
        ok=sa3ds_parity_trace_write_frame(output,SA3DS_PARITY_ZONE_CALIBRATION,
            name,&player,&input,step.camera_yaw,&debug,&selection)==0;
    }
    if(fclose(output))ok=0;
    sa3ds_ec_collision_destroy(&collision);free(bytes);
    if(!ok)return 1;
    printf("WCA host replay: %u controller ticks,152 cases; native comparison only, NOT RETAIL VERIFIED\n",SA3DS_WCA_REPLAY_TICKS);
    return 0;
}
