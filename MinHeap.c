/**
 * @file		MinHeap.c
 * @brief		A min heap for unsigned 32 bit integer elements.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdio.h>
#include <stdlib.h>

#include "MinHeap.h"

struct HeapNode {
	u32 elem;
	u32 weight;
};
typedef struct HeapNode HeapNode;

struct MinHeap {
	HeapNode* data;
	u32 maxSize;
	u32 currSize;
};

inline static u32 parent(u32 index) {
	return (index - 1) >> 1;
}

inline static u32 lchild(u32 index) {
	return (index << 1) + 1;
}

inline static u32 rchild(u32 index) {
	return (index << 1) + 2;
}

MinHeap* newMinHeap(u32 size) {
	MinHeap* heap = malloc(sizeof(MinHeap));
	heap->data = malloc(sizeof(HeapNode) * size);
	heap->maxSize = size;
	heap->currSize = 0;
	return heap;
}

void deleteMinHeap(MinHeap* heap) {
	if (heap == NULL) return;
	if (heap->data != NULL) free(heap->data);
	free(heap);
}

u32 heapElementCount(const MinHeap* heap) {
	return heap->currSize;
}

inline static void swap(MinHeap* heap, u32 ia, u32 ib) {
	HeapNode temp = heap->data[ia];
	heap->data[ia] = heap->data[ib];
	heap->data[ib] = temp;
}

static void bubbleUp(MinHeap* heap, u32 index) {
	while (index > 0 && heap->data[parent(index)].weight > heap->data[index].weight) {
		swap(heap, parent(index), index);
		index = parent(index);
	}
}

static void bubbleDown(MinHeap* heap, u32 index) {
	while (true) {
		u32 left = lchild(index);
		u32 right = rchild(index);
		u32 lweight = left < heap->currSize ? heap->data[left].weight : UINT32_MAX;
		u32 rweight = right < heap->currSize ? heap->data[right].weight : UINT32_MAX;
		u32 sindex = (lweight < rweight) ? left : right;
		u32 sweight = (lweight < rweight) ? lweight : rweight;
		if (heap->data[index].weight > sweight) {
			swap(heap, index, sindex);
			index = sindex;
		} else {
			break;
		}
	}
}

bool heapPopMin(MinHeap* heap, u32* elem, u32* weight) {
	if (heap->currSize == 0) return false;

	*elem = heap->data[0].elem;
	*weight = heap->data[0].weight;
	swap(heap, 0, heap->currSize - 1);

	--(heap->currSize);
	bubbleDown(heap, 0);
	return true;
}

bool heapInsert(MinHeap* heap, u32 elem, u32 weight) {
	if (heap->currSize == heap->maxSize) return false;

	heap->data[heap->currSize].elem = elem;
	heap->data[heap->currSize].weight = weight;

	u32 index = (heap->currSize)++;
	bubbleUp(heap, index);
	return true;
}
