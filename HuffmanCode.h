/**
 * @file		HuffmanCode.h
 * @brief		The interface to the huffman encoder/decoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef HUFFMAN_CODE_H_INCLUDED
#define HUFFMAN_CODE_H_INCLUDED

#include "CommonDef.h"
#include "ByteArray.h"

ByteArray* huffmanDecode(const wchar_t* treeName, const byte* compressedData, u32 compressedLen, u32 originalLen);
ByteArray* huffmanEncode(const wchar_t* treeName, const byte* originalData, u32 originalLen);

#endif
