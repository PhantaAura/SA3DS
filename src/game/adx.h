#ifndef SA3DS_ADX_H
#define SA3DS_ADX_H
#include <stddef.h>
#include <stdint.h>
/*CRI ADX (encoding 3, 4-bit, 18-byte blocks, version 3/4 headers) as used by
 *the owner's PC2004 BGM. Follows the published format and vgmstream's
 *conventions (block scale+1, coef1=floor(c*8192), coef2=floor(-c*c*4096),
 *high nibble first, interleaved channel blocks, loop history saved when the
 *loop start is first decoded). Not compared against a reference decoder.*/
typedef struct {
 uint32_t data_offset;   /*copyright offset+4*/
 uint8_t block_size,channels;
 uint32_t rate,total_samples;
 uint8_t loop;           /*header loop flag*/
 uint32_t loop_start,loop_end; /*samples*/
 uint32_t loop_start_byte;
 int32_t coef1,coef2;
} SA3DSAdxInfo;
typedef struct {
 int32_t h1[2],h2[2];
 int32_t loop_h1[2],loop_h2[2];
 uint32_t sample;        /*next sample index*/
 uint8_t loop_saved;
} SA3DSAdxState;
/*Parses the header (needs at least data_offset bytes). Returns1 on success.*/
int sa3ds_adx_open(SA3DSAdxInfo *info,const uint8_t *header,size_t size);
/*Decodes one frame (channels*block_size bytes) into 32*channels interleaved
 *PCM16 samples and advances state. Saves loop history at loop_start.*/
int sa3ds_adx_decode_frame(const SA3DSAdxInfo *info,SA3DSAdxState *state,const uint8_t *frame,int16_t *out);
/*Loop jump: restores the saved history and sets the sample index. Returns the
 *byte offset to resume reading from, or 0 when no loop history exists.*/
uint32_t sa3ds_adx_loop(const SA3DSAdxInfo *info,SA3DSAdxState *state);
/*"archive  V2.DMZ" SE bank (PC2004 SoundData/SE banks): u32 count at +0x10,
 *then count x {name offset, data offset, size}; entries are ADX despite the
 *.WAV names. Returns1 and fills the entry when index is valid.*/
typedef struct {const char *name;const uint8_t *data;uint32_t size;} SA3DSSoundBankEntry;
uint32_t sa3ds_sound_bank_count(const uint8_t *bank,size_t size);
int sa3ds_sound_bank_entry(const uint8_t *bank,size_t size,uint32_t index,SA3DSSoundBankEntry *out);
#endif
