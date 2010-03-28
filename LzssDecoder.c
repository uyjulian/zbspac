/**
 * @file		LzssDecoder.c
 * @brief		The implementation of the LZSS decoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.03
 */

#include "LzssCode.h"
#include "Logger.h"

#define N 4096
#define F 18

static inline byte getBit(byte data, byte offset) {
	return (data >> (7 - offset)) & 1;
}

ByteArray* lzssDecode(const byte* encodedData, u32 encodedLen, u32 decodedLen) {
	u32 enIndex = 0;
	u32 deIndex = 0;
	ByteArray* result = newByteArray(decodedLen);
	byte* decodedData = baData(result);

	byte text_buf[N + F - 1];
	int i, j, k, r;
	byte c = 0;
	byte flags;

	for (i = 0; i < N - F; i++)
		text_buf[i] = '\0';
	r = 4078;
	flags = 0;
	for (;;) {
		if (enIndex == encodedLen) break;
		flags = encodedData[enIndex++];
		for (int curbit = 7; curbit >= 0; --curbit) {
			if (getBit(flags, curbit)) {
				if (enIndex == encodedLen || deIndex == decodedLen) goto out;
				c = encodedData[enIndex++];
				decodedData[deIndex++] = c;
				text_buf[r++] = c;
				r &= (N - 1);
			} else {
				if (enIndex == encodedLen) goto out;
				i = encodedData[enIndex++];
				if (enIndex == encodedLen) goto out;
				j = encodedData[enIndex++];

				i |= (j >> 4) << 8;
				j = (j & 0x0f) + 3;
				for (k = 0; k < j; k++) {
					if (deIndex == decodedLen) goto out;
					c = text_buf[(i + k) & (N - 1)];
					decodedData[deIndex++] = c;
					text_buf[r++] = c;
					r &= (N - 1);
				}
			}
		}
	}
	out:
		return result;
}
