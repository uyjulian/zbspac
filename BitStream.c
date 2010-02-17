/**
 * @file		BitStream.c
 * @brief		A data stream that operates on bits.
 * 				It is basically a "bit-oriented view" of an underlying byte array,
 * 				so any manipulation on the stream will affect that array.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>

#include "BitStream.h"

struct BitStream {
	byte* data;
	u32 dataLen;
	byte currBitIndex;
	u32 currByteIndex;
};

BitStream* newBitStream(byte* bytes, u32 length) {
	BitStream* bs = malloc(sizeof(BitStream));
	bs->data = bytes;
	bs->dataLen = length;
	bs->currBitIndex = 0;
	bs->currByteIndex = 0;
	return bs;
}

void deleteBitStream(BitStream* bs) {
	if (!bs) return;
	free(bs);
	bs = NULL;
}

bool bsNextBit(BitStream* bs, byte* result) {
	if (bs->currByteIndex == bs->dataLen) return false;
	*result = getBit(bs->data[bs->currByteIndex], (bs->currBitIndex)++);
	if (bs->currBitIndex == 8) {
		bs->currBitIndex = 0;
		++(bs->currByteIndex);
	}
	return true;
}

bool bsNextByte(BitStream* bs, byte* result) {
	if (bs->currBitIndex > 0 && bs->currByteIndex == bs->dataLen - 1)
		return false;

	if (bs->currBitIndex == 0) {
		*result = bs->data[(bs->currByteIndex)++];
		return true;
	}

	byte part1 = bs->data[bs->currByteIndex] << bs->currBitIndex;
	byte part2 =  bs->data[++(bs->currByteIndex)] >> (8 - bs->currBitIndex);
	*result = part1 | part2;
	return true;
}

byte getBit(byte data, byte offset) {
	return (data >> (7 - offset)) & 1;
}
