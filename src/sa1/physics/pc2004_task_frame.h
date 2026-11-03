#ifndef SA1_PC2004_TASK_FRAME_H
#define SA1_PC2004_TASK_FRAME_H
#include "surface_frame.h"
#include "pc2004_homing.h"
/*43EC90 physical rotation Rz(Z)*Rx(X)*Ry(-Y): columns are forward,up,
 * negative port-right. Invert its canonical principal-X representation.
 * At the source4B8E80 normal pole Z=0. Inverse quantization rounds a stored
 * rotation field; it is not the source target-bearing truncation operation.
 * Original angle history/smoothing and exceptional Euler branches remain
 * caller-owned; this does not prove that a port contact frame equals retail. */
int sa1_pc2004_task_yaw(const SA1SurfaceFrame *frame,uint16_t *yaw);
/*Apply raw task-Y delta holding the physical up/X/Z rotation fixed.*/
int sa1_pc2004_task_turn(SA1SurfaceFrame *frame,uint16_t old_yaw,uint16_t new_yaw,
    const SA1PC2004HomingMath *math);
/*443AD0 default-world-gravity canonical X/Z approach by800 BAMS. Caller
 * retains world velocity and task foot origin while changing this basis. */
int sa1_pc2004_task_relax_up(SA1SurfaceFrame *frame,const SA1PC2004HomingMath *math);
#endif
