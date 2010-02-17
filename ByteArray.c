/**
 * @file		ByteArray.c
 * @brief		A heap-allocated byte array that knows how long itself is.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <string.h>

#include "ByteArray.h"

struct ByteArray {
	byte* data;
	u32 length;
};

ByteArray* newByteArray(u32 length) {
	ByteArray* array = malloc(sizeof(ByteArray));
	array->data = malloc(sizeof(byte) * length);
	memset(array->data, 0, length);
	array->length = length;
	return array;
}

void deleteByteArray(ByteArray* array) {
	if (!array) return;
	if (array->data) free(array->data);
	free(array);
	array = NULL;
}

byte* baData(const ByteArray* array) {
	return array->data;
}

u32 baLength(const ByteArray* array) {
	return array->length;
}
