/**
 * @file		LzssDecoder.c
 * @brief		The implementation of the LZSS decoder (NeXaS flavor).
 * 				This is actually a modified version of Mr. Haruhiko Okumura's original code.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.03
 */

#include "LzssCode.h"
#include "Logger.h"

#define WINDOW_SIZE 4096
#define MAX_MATCH_LENGTH 18
#define THRESHOLD 2

static inline byte getBit(byte data, byte offset) {
	return (data >> (7 - offset)) & 1;
}

ByteArray* lzssDecode(const byte* encodedData, u32 encodedLen, u32 decodedLen) {
	u32 enIndex = 0;
	u32 deIndex = 0;
	ByteArray* result = newByteArray(decodedLen);
	byte* decodedData = baData(result);

	byte window[WINDOW_SIZE];
	for (u32 i = 0; i < WINDOW_SIZE; i++) window[i] = '\0';

	u32 winIndex = WINDOW_SIZE - MAX_MATCH_LENGTH;

	for (;;) {
		if (enIndex == encodedLen) break;
		byte flags = encodedData[enIndex++];
		for (int curbit = 7; curbit >= 0; --curbit) {
			if (getBit(flags, curbit)) {
				if (enIndex == encodedLen || deIndex == decodedLen) goto out;
				byte data = encodedData[enIndex++];
				decodedData[deIndex++] = data;
				window[winIndex++] = data;
				winIndex &= (WINDOW_SIZE - 1);
			} else {
				if (enIndex == encodedLen) goto out;
				u32 position = encodedData[enIndex++];
				if (enIndex == encodedLen) goto out;
				u32 length = encodedData[enIndex++];

				position |= (length >> 4) << 8;
				length = (length & 0x0f) + THRESHOLD + 1;
				for (u32 i = 0; i < length; ++i) {
					if (deIndex == decodedLen) goto out;
					byte data = window[(position + i) & (WINDOW_SIZE - 1)];
					decodedData[deIndex++] = data;
					window[winIndex++] = data;
					winIndex &= (WINDOW_SIZE - 1);
				}
			}
		}
	}
	out:
		return result;
}
