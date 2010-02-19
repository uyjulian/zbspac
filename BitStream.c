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

static inline byte getBit(byte data, byte offset) {
	return (data >> (7 - offset)) & 1;
}

static inline void setBit(byte* data, byte offset, byte value) {
	byte mask = 1 << (7 - offset);
	value ? (*data |= mask) : (*data &= (mask ^ 0xFF));
}

BitStream* newBitStream(byte* data, u32 length) {
	BitStream* bs = malloc(sizeof(BitStream));
	bs->dataLen = length;
	bs->data = data;
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

bool bsSetNextBit(BitStream* bs, byte data) {
	if (bs->currByteIndex == bs->dataLen) return false;
	setBit(&(bs->data[bs->currByteIndex]), (bs->currBitIndex)++, data);
	if (bs->currBitIndex == 8) {
		bs->currBitIndex = 0;
		++(bs->currByteIndex);
	}
	return true;
}

bool bsSetNextByte(BitStream* bs, byte data) {
	if (bs->currBitIndex > 0 && bs->currByteIndex == bs->dataLen - 1)
		return false;

	if (bs->currBitIndex == 0) {
		bs->data[(bs->currByteIndex)++] = data;
		return true;
	}
	byte offset = bs->currBitIndex;
	byte part1 = data >> offset;
	byte part2 = data << (8 - offset);
	bs->data[bs->currByteIndex] = ((bs->data[bs->currByteIndex] >> (8 - offset)) << (8 - offset)) | part1;
	bs->data[bs->currByteIndex + 1] = ((bs->data[bs->currByteIndex + 1] << offset) >> offset) | part2;
	++bs->currByteIndex;
	return true;
}

u32 bsGetCurrentByteIndex(const BitStream* bs) {
	return bs->currByteIndex;
}

u32 bsGetCurrentBitIndex(const BitStream* bs) {
	return bs->currBitIndex;
}
