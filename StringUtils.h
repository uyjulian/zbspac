/**
 * @file		WCString.h
 * @brief		"I18N" support and utility functions for heap-allocated strings.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef STRING_UTILS_H_INCLUDED
#define STRING_UTILS_H_INCLUDED

#include "CommonDef.h"

wchar_t* newWCString(u32 size);
wchar_t* cloneWCString(const wchar_t* src);
wchar_t* toWCString(const char* mbs, const wchar_t* locale);
char* toMBString(const wchar_t* wcs, const wchar_t* locale);

wchar_t* wcsAppend(const wchar_t* first, const wchar_t* second);
wchar_t* wcsSubstring(const wchar_t* src, u32 startIndex, u32 endIndex);
i32 wcsFindChar(const wchar_t* str, wchar_t target, bool forward);

#endif
