#ifndef SA3DS_PC2004_LIGHTING_H
#define SA3DS_PC2004_LIGHTING_H
#include <stddef.h>
#include <stdint.h>
typedef struct {const uint8_t*stage;const uint8_t*character[2];} SA3DSLightingAssets;
/*Borrowed ordinary EC/Sonic mode2 settings, not all render-state/event services.*/
int sa3ds_lighting_assets_open(SA3DSLightingAssets*out,const void*bytes,size_t size);
/*D3DLIGHT8 API snapshot, type3 and source floats retained without normalization.*/
typedef struct {uint32_t type;float diffuse[4],specular[4],ambient[4],position[3],direction[3],range,falloff,attenuation[3],theta,phi;} SA3DSLight8;
int sa3ds_ec_character_lights(const SA3DSLightingAssets*assets,unsigned act,
    SA3DSLight8 lights[4],uint8_t*enabled_mask);
/*784850 ordinary Basic material output AFTER caller resolves constant material.
 * Source converts byte components to floats0..255; it does not divide by255.
 * Effective COLORVERTEX/material-source/FVF policy belongs to the renderer.*/
typedef struct {float diffuse[4],ambient[4],specular[4],emissive[4],power;} SA3DSMaterial8;
int sa3ds_pc2004_basic_material8(uint32_t resolved_diffuse,uint32_t specular,
    float power,SA3DSMaterial8*out);
int sa3ds_pc2004_normal_matrix(const float actor[16],const float node[16],float out[9]);
/*Directional fixed-function candidate. Normal is already inverse-transpose
 * transformed; normalization and inherited global ambient are explicit inputs.
 * EC Basic uses COLOR1 for diffuse/ambient, MATERIAL for specular.*/
int sa3ds_pc2004_light_vertex(const SA3DSLight8 lights[4],uint8_t enabled_mask,
    const SA3DSMaterial8 *material,const float color1[4],const float global_ambient[3],
    const float normal[3],const float position[3],const float eye[3],int normalize_normal,
    float diffuse[4],float specular[4]);
#endif
