/**
 * @file		ByteArray.h
 * @brief		A heap-allocated byte array that knows how long itself is.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef BYTE_ARRAY_H_INCLUDED
#define BYTE_ARRAY_H_INCLUDED

#include "CommonDef.h"

struct ByteArray;
typedef struct ByteArray ByteArray;

ByteArray* newByteArray(u32 length);
void deleteByteArray(ByteArray* array);

byte* baData(const ByteArray* array);
u32 baLength(const ByteArray* array);

#endif
