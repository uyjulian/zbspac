/**
 * @file		LzssCode.h
 * @brief		The interface to the LZSS encoder/decoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.03
 */

#ifndef LZSS_CODE_H_INCLUDED
#define LZSS_CODE_H_INCLUDED

#include "CommonDef.h"
#include "ByteArray.h"

ByteArray* lzssDecode(const byte* compressedData, u32 compressedLen, u32 originalLen);
ByteArray* lzssEncode(const byte* originalData, u32 originalLen);

#endif
