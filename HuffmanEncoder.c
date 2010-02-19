/**
 * @file		HuffmanEncoder.c
 * @brief		The implementation of the huffman encoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <string.h>

#include "MinHeap.h"
#include "Logger.h"
#include "BitStream.h"
#include "HuffmanCode.h"

struct TreeNode {
	u16 parent;
	u16 isrchild;
	u16 lchild;
	u16 rchild;
	u32 weight;
};
typedef struct TreeNode TreeNode;

static u32 buildTree(const wchar_t* treeName, TreeNode tree [], const byte* originalData, u32 originalLen) {
	writeLog(LOG_VERBOSE, L"Counting byte values.......");
	/// The first 256 nodes are leaves that represent byte values.
	for (u32 i = 0; i < originalLen; ++i) {
		++(tree[originalData[i]].weight);
	}

	/// Generate the other nodes.
	MinHeap* heap = newMinHeap(256);
	for (u16 i = 0; i < 256; ++i) {
		/// Byte values that don't appear in the data are ignored.
		if (tree[i].weight > 0) {
			heapInsert(heap, i, tree[i].weight);
			writeLog(LOG_VERBOSE, L"  Byte: %x, Count: %u", i, tree[i].weight);
		}
	}

	writeLog(LOG_VERBOSE, L"Generating Tree.......");
	u16 index = 256;
	while (heapElementCount(heap) > 1) {
		u32 ia, ib, wa, wb;
		heapPopMin(heap, &ia, &wa);
		heapPopMin(heap, &ib, &wb);
		tree[index].lchild = ia;
		tree[index].rchild = ib;
		tree[index].weight = wa + wb;
		tree[ia].parent = index;
		tree[ia].isrchild = false;
		tree[ib].parent = index;
		tree[ib].isrchild = true;
		heapInsert(heap, index, wa + wb);
	}
	u32 rootIndex, rootWeight;
	heapPopMin(heap, &rootIndex, &rootWeight);
	deleteMinHeap(heap);
	writeLog(LOG_VERBOSE, L"Tree root is at Index %u.", rootIndex);
	return rootIndex;
}

static void encodeTree(const wchar_t* treeName, TreeNode tree[], u32 rootIndex, BitStream* bs) {
	writeLog(LOG_VERBOSE, L"Encoding the tree itself......");
	if (rootIndex < 256) {
		bsSetNextBit(bs, 0);
		bsSetNextByte(bs, rootIndex);
	} else {
		bsSetNextBit(bs, 1);
		encodeTree(treeName, tree, tree[rootIndex].lchild, bs);
		encodeTree(treeName, tree, tree[rootIndex].rchild, bs);
	}
	writeLog(LOG_VERBOSE, L"Tree Encoded.");
}

static void encodeData(const wchar_t* treeName, TreeNode tree[], u32 rootIndex, const byte* data, u32 oriLen, BitStream* bs) {
	writeLog(LOG_VERBOSE, L"Encoding data.......");
	u8 code[256][256];
	u8 codeLen[256];
	memset(code, 0, 256 * 256);
	memset(codeLen, 0, 256);

	for (u16 i = 0; i < 256; ++i) {
		if (tree[i].weight == 0) continue;
		u16 index = i;
		while (index != rootIndex) {
			code[i][codeLen[i]++] = tree[index].isrchild;
			index = tree[index].parent;
		}
	}

	for (u32 i = 0; i < oriLen; ++i) {
		u8* thisCode = code[data[i]];
		for (u16 j = codeLen[data[i]] - 1; j >= 0; --j) {
			bsSetNextBit(bs, thisCode[j]);
		}
	}
	writeLog(LOG_VERBOSE, L"Data Encoded.");
}

ByteArray* huffmanEncode(const wchar_t* treeName, const byte* originalData, u32 originalLen) {
	writeLog(LOG_VERBOSE, L"Generating Huffman Codes for: %s", treeName);
	/// At most 256 leaves, and 255 internal nodes, but 512 just looks nicer. ;)
	TreeNode tree[512];
	memset(tree, 0, sizeof(tree));
	u32 rootIndex = buildTree(treeName, tree, originalData, originalLen);
	ByteArray* encodedData = newByteArray(2 * originalLen);
	BitStream* encodedStream = newBitStream(baData(encodedData), 2 * originalLen);
	encodeTree(treeName, tree, rootIndex, encodedStream);
	encodeData(treeName, tree, rootIndex, originalData, originalLen, encodedStream);
	u32 encodedLen = bsGetCurrentByteIndex(encodedStream) + 1;
	ByteArray* resultData = newByteArray(encodedLen);
	memcpy(baData(resultData), baData(encodedData), encodedLen);
	deleteBitStream(encodedStream);
	deleteByteArray(encodedData);
	writeLog(LOG_VERBOSE, L"Generated Huffman Codes for: %s", treeName);
	return resultData;
}
