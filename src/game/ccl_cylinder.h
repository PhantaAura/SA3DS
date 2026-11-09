#ifndef SA3DS_CCL_CYLINDER_H
#define SA3DS_CCL_CYLINDER_H
#include "ccl_contact.h"
/*41CCD0: primary form7 (first; world-Y gravity, centre task+(0,b,0) per
 *41C5D0, radius a, half-height b, foot radius c) against a form1 vertical
 *cylinder (second; world centre, radius a, half-height b). Branches:
 * top  : pbot<ctop<0.4*ptop+0.6*pbot and d^2<(c+ca)^2 -> CCL flags first|8,
 *        second|2; when first is pushed (418D80): task status first|0x20
 *        (ground, player0 only via 41CC40: caller flag), second|0x10, first
 *        rot x/z cleared, first y += ctop-0.01-pbot.
 * side : d^2<(pa+ca)^2, not in cbot<ptop<cbot+2, pbot<ctop, cbot<ptop and
 *        split<ctop -> both CCL flags|4; if |d|-pa-ca<0 the horizontal normal
 *        (or (0,0.1) when d=0) is stored and a pushed first moves by
 *        n*((pa+ca-|d|)-0.1)*0.5.
 * head : cbot<ptop<cbot+2 -> first|2, second|8; pushed first y += cbot+0.01-ptop,
 *        task status first|0x10 (player0), second|0x20.
 *Returns 0 miss, 1 top, 2 side, 3 head, -1 invalid. Contact bookkeeping uses
 *sa3ds_ccl_record_contact (418E70/418DE0).*/
typedef struct {float half_height,foot_radius;} SA3DSCCLPrimaryExtent;
int sa3ds_ccl_cylinder_contact(SA3DSCCLBody *first,SA3DSCCLShape *first_shape,unsigned first_index,const SA3DSCCLPrimaryExtent *extent,
 SA3DSCCLBody *second,SA3DSCCLShape *second_shape,unsigned second_index,float second_half_height,int first_is_player0,SA1Vec3 *first_position,int *clear_first_tilt);
#endif
