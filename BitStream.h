/**
 * @file		BitStream.h
 * @brief		A data stream that operates on bits.
 * 				It is basically a "bit-oriented view" of an underlying byte array,
 * 				so any manipulation on the stream will affect that array.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef BIT_STREAM_H_INCLUDED
#define BIT_STREAM_H_INCLUDED

#include "CommonDef.h"

struct BitStream;
typedef struct BitStream BitStream;

BitStream* newBitStream(byte* data, u32 length);
void deleteBitStream(BitStream* bs);

bool bsNextBit(BitStream* bs, byte* result);
bool bsNextByte(BitStream* bs, byte* result);

bool bsSetNextBit(BitStream* bs, byte data);
bool bsSetNextByte(BitStream* bs, byte data);

u32 bsGetCurrentByteIndex(const BitStream* bs);
u32 bsGetCurrentBitIndex(const BitStream* bs);

#endif
