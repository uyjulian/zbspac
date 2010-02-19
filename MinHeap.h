/**
 * @file		MinHeap.h
 * @brief		A generic min heap.
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
bool heapPopMin(MinHeap* heap, HEAP_ELEM_T* elem, u32* weight);
bool heapInsert(MinHeap* heap, HEAP_ELEM_T elem, u32 weight);

#endif
