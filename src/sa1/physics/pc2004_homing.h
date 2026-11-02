#ifndef SA1_PC2004_HOMING_H
#define SA1_PC2004_HOMING_H
#include "pc2004_air_attack.h"
#include "player_state.h"
#define SA1_PC2004_HOMING_CAPACITY 0x290u
typedef struct {uint32_t actor_id;float task_distance_squared;} SA1PC2004HomingCandidate;
typedef struct {unsigned count;SA1PC2004HomingCandidate entries[SA1_PC2004_HOMING_CAPACITY];} SA1PC2004HomingList;
typedef struct {SA1Vec3 task_origin,first_center;uint32_t first_shape_flags;} SA1PC2004HomingPose;
typedef int (*SA1PC2004HomingResolve)(void *context,uint32_t actor_id,SA1PC2004HomingPose *out);
typedef struct {
    void *context;
    /* Source scales atan2 by65536 then inverse-tau .15915497620314795,
     * not mathematical1/(2*pi), and truncates before low16 extraction. */
    uint16_t (*bearing)(void *context,float z,float x);
    double (*sine)(void *context,uint16_t angle);
    uint16_t (*random15)(void *context);
} SA1PC2004HomingMath;
typedef struct {
    const SA1PC2004HomingList *list;
    SA1Vec3 player_task_origin;
    SA1PC2004LocalVector local_speed;
    uint16_t task_yaw;
    uint32_t partner_id; /*0 means source441BF0 returned-1. */
    int special_player; /*player-work8000, not task status. */
    SA1PC2004HomingResolve resolve;
    void *resolve_context;
    SA1PC2004HomingMath math;
} SA1PC2004HomingInput;
typedef enum {SA1_PC2004_HOMING_ACCEL_ONLY,SA1_PC2004_HOMING_DECAY_ACCEL,SA1_PC2004_HOMING_TRACK} SA1PC2004HomingMode;
typedef struct {
    SA1PC2004AirAttack next;
    SA1PC2004HomingMode mode;
    uint16_t task_yaw;
    uint32_t selected_id;
    SA1PC2004LocalVector local_speed; /*before44C270, when mode!=TRACK */
    SA1Vec3 world_velocity; /*TRACK: caller applies new task yaw, then43EC00 */
} SA1PC2004HomingResult;
void sa1_pc2004_homing_reset(SA1PC2004HomingList *list);
/*418D00/418BE0: range uses task positions and is independent of body overlap.
 * Caller supplies actual player identity, CCL flags and task ID, in CCL order.
 * ID11 rings belong to a separate source table and are excluded here.
 * Returns1 appended,0 filtered/full,-1 invalid inputs. IDs are pointer-free. */
int sa1_pc2004_homing_collect(SA1PC2004HomingList *list,int player_index,
    unsigned player_ccl_category,unsigned target_ccl_flags,uint8_t target_task_id,
    uint32_t actor_id,SA1Vec3 player_task_origin,SA1Vec3 target_task_origin);
/*492300, after the action selector; does not integrate position or acceleration.
 * Resolve CURRENT actor/first-record poses; list distances come from prior CCL.
 * Inject source bearing/trig and RNG. The port does not substitute nearest
 * center distance, instant yaw, generic randomness or clamped upward pursuit. */
int sa1_pc2004_homing_step(const SA1PC2004AirAttack *state,
    const SA1PC2004HomingInput *input,SA1PC2004HomingResult *out);
unsigned sa1_pc2004_homing_angle_distance(uint16_t from,uint16_t to);
#endif
