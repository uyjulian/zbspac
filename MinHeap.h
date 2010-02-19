/**
 * @file		MinHeap.h
 * @brief		A min heap for unsigned 32 bit integer elements.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef MIN_HEAP_H_INCLUDED
#define MIN_HEAP_H_INCLUDED

#include "CommonDef.h"

struct MinHeap;
typedef struct MinHeap MinHeap;

MinHeap* newMinHeap(u32 size);
void deleteMinHeap(MinHeap* heap);

u32 heapElementCount(const MinHeap* heap);
bool heapPopMin(MinHeap* heap, u32* elem, u32* weight);
bool heapInsert(MinHeap* heap, u32 elem, u32 weight);

#endif
