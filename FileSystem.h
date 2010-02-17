/**
 * @file		FileSystem.h
 * @brief		Directory and path manipulation functions.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED

#include "CommonDef.h"

wchar_t* fsAbsolutePath(const wchar_t* relativePath);
wchar_t* fsCombinePath(const wchar_t* directory, const wchar_t* filename);
bool fsEnsureDirectoryExists(const wchar_t* dir);

#endif
