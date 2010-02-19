/**
 * @file		HuffmanDecoder.c
 * @brief		The implementation of the huffman decoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "BitStream.h"
#include "HuffmanCode.h"

/**
 * Using an compact array representation of the huffman tree,
 * which can have at most 256 leaves, and 256 - 1 = 255 internal nodes.
 * As the only valuable information about leaves are the byte value
 * they represent, we can just store that value in their parents' lchild
 * and rchild links. So these links serve two purposes, either they are
 * links to child internal nodes (in this case their value will be the
 * child's index) or they represent the child leaf nodes' value.
 * (between 0 + 1024 and 255 + 1024)
 */

struct TreeNode {
	u16 lchild;
	u16 rchild;
};
typedef struct TreeNode TreeNode;

static bool subTreeCreationWorker(const wchar_t* treeName, TreeNode tree[], BitStream* bs, u16* subTreeRoot, u16* freeSlotIndex) {
	byte rbyte = 0;
	if (!bsNextBit(bs, &rbyte)) {
		writeLog(LOG_QUIET, L"ERROR: Unable to generate huffman tree for %s: encoded data exhausted!", treeName);
		return false;
	}
	if (rbyte) {
		/**
		 * An '1' means we should recursively generate the subtrees of the
		 * current node (preorder traversal indeed).
		 * "Allocate space" for the current node then recur.
		 */
		*subTreeRoot = (*freeSlotIndex)++;
		if (*subTreeRoot == 256) {
			writeLog(LOG_QUIET, L"ERROR: Unable to generate huffman tree for %s: encoded data corrupted!", treeName);
			return false;
		}
		u16 childRoot = 0;
		if (!subTreeCreationWorker(treeName, tree, bs, &childRoot, freeSlotIndex))
			return false;
		tree[*subTreeRoot].lchild = childRoot;
		if (!subTreeCreationWorker(treeName, tree, bs, &childRoot, freeSlotIndex))
			return false;
		tree[*subTreeRoot].rchild = childRoot;
		return true;
	} else {
		/**
		 * A '0' means the current node is a leaf node and the 8-bit
		 * data following is the byte value of this leaf.
		 * A Leaf node's value is stored directly in its parent's links.
		 * So for this subtree we just return the byte value + 1024.
		 */
		if (!bsNextByte(bs, &rbyte)) {
			writeLog(LOG_QUIET,L"ERROR: Cannot generate huffman tree for %s: encoded data exhausted!", treeName);
			return false;
		}
		*subTreeRoot = rbyte + 1024;
		return true;
	}
	return false;
}

static bool createTree(const wchar_t* treeName, TreeNode tree[], BitStream* bs) {
	writeLog(LOG_VERBOSE, L"Creating huffman tree for %s...", treeName);
	u16 treeRoot = 0;
	u16 freeSlotIndex = 0;
	subTreeCreationWorker(treeName, tree, bs, &treeRoot, &freeSlotIndex);
	/**
	 * The return tree root will not be 0 if it is a tree with only one node,
	 * which means the tree is corrupted.
	 * The freeSlotIndex doesn't have to be 256 in the end, as the original
	 * data may not contain all 256 byte values, and the tree will be smaller,
	 * thus taking up less than 255 slots.
	 */
	if (treeRoot != 0) {
		writeLog(LOG_QUIET, L"ERROR: Cannot generate huffman tree for %s: encoded data exhausted!", treeName);
		return false;
	}
	writeLog(LOG_VERBOSE, L"The huffman tree for %s is created, node count: %u.", treeName, freeSlotIndex);
	return true;
}

static ByteArray* decodeWithTree(const wchar_t* treeName, TreeNode tree[], BitStream* data, u32 originalLen) {
	u16 treeIndex = 0;
	u32 resIndex = 0;
	byte rbyte = 0;
	ByteArray* result = newByteArray(originalLen);

	while (true) {
		if (!bsNextBit(data, &rbyte)) {
			writeLog(LOG_QUIET, L"ERROR: Cannot decode the huffman code for %s: encoded data exhausted!", treeName);
			return false;
		}

		if (rbyte) {
			treeIndex = tree[treeIndex].rchild;
		} else {
			treeIndex = tree[treeIndex].lchild;
		}

		if (treeIndex >= 1024) {
			/// Byte value
			baData(result)[resIndex++] = treeIndex - 1024;
			treeIndex = 0;

			/**
			 * The encoded data may not take up whole bytes, so there would be
			 * some unused bits in the source array. To determine whether we have
			 * decoded all the data we needed, we just do the following:
			 */
			if (resIndex == originalLen) {
				return result;
			}
		}
	}
	return NULL;
}

ByteArray* huffmanDecode(const wchar_t* treeName, const byte* compressedData, u32 compressedLen, u32 originalLen) {
	TreeNode tree[256];
	BitStream* bs = newBitStream((byte*)compressedData, compressedLen);
	memset(tree, 0, sizeof(TreeNode) * 256);
	if (!createTree(treeName, tree, bs)) {
		deleteBitStream(bs);
		return NULL;
	}
	ByteArray* result = decodeWithTree(treeName, tree, bs, originalLen);
	deleteBitStream(bs);
	return result;
}
