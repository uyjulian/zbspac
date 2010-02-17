/**
 * @file		StringUtils.c
 * @brief		"I18N" support and utility functions for heap-allocated strings.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include "StringUtils.h"

void encodingSwitchToNative() {
	setlocale(LC_ALL, ".ACP");
}

void encodingSwitchToShiftJIS() {
	setlocale(LC_ALL, "japanese");
}

char* newMBString(u32 length) {
	return malloc(sizeof(char) * (length + 1));
}

wchar_t* newWCString(u32 length) {
	return malloc(sizeof(wchar_t) * (length + 1));
}

wchar_t* cloneWCString(const wchar_t* src) {
	 wchar_t* result  = malloc(sizeof(wchar_t) * (wcslen(src) + 1));
	 wcscpy(result, src);
	 return result;
}

wchar_t* toWCString(const char* mbs) {
	if (mbs == NULL) return NULL;
	u32 len = mbstowcs(NULL, mbs, CBUF_TRY_SIZE);
	wchar_t* result = malloc(sizeof(wchar_t) * (len + 1));
	mbstowcs(result, mbs, len + 1);
	return result;
}

char* toMBString(const wchar_t* wcs) {
	if (wcs == NULL) return NULL;
	u32 len = wcstombs(NULL, wcs, CBUF_TRY_SIZE);
	char* result = malloc(sizeof(char) * (len + 1));
	wcstombs(result, wcs, len + 1);
	return result;
}

wchar_t* wcsAppend(const wchar_t* first, const wchar_t* second) {
	if (first == NULL) return cloneWCString(second);
	if (second == NULL) return cloneWCString(first);

	u32 firstLen = wcslen(first);
	u32 totalLen = firstLen + wcslen(second);
	wchar_t* result = newWCString(totalLen);
	wcscpy(result, first);
	wcscpy(result + firstLen, second);
	return result;
}

wchar_t* wcsSubstring(const wchar_t* src, u32 startIndex, u32 endIndex) {
	if (src == NULL || endIndex < startIndex) return NULL;

	u32 srcLen = wcslen(src);

	if (endIndex == 0)
		endIndex = srcLen;
	else if (srcLen < endIndex)
		endIndex = srcLen;

	u32 length = endIndex - startIndex;
	wchar_t* result = newWCString(length);
	wcsncpy(result, src + startIndex, length);
	result[length] = L'\0';

	return result;
}

i32 wcsFindChar(const wchar_t* str, wchar_t target, bool forward) {
	if (str == NULL) return -1;
	i32 start, end, step;

	u32 length = wcslen(str);
	if (forward) {
		start = 0; end = length; step = 1;
	} else {
		start = length - 1; end = -1; step = -1;
	}

	for (i32 i = start; i != end; i += step) {
		if (str[i] == target) {
			return i;
		}
	}
	return -1;
}
