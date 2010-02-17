/**
 * @file		Logger.c
 * @brief		Implementation of "the" simple logger.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

#include "Logger.h"

static LogLevel logLevel;

void setLogLevel(LogLevel level) {
	logLevel = level;
}

void writeLog(LogLevel level, const wchar_t* str, ...) {
	if (level > logLevel) return;

	va_list args;
	va_start(args, str);
	vfwprintf(stderr, str, args);
	va_end(args);

	fputwc(L'\n', stderr);
}

void writeOnlyOnLevel(LogLevel level, const wchar_t* str, ...) {
	if (level != logLevel) return;

	va_list args;
	va_start(args, str);
	vfwprintf(stderr, str, args);
	va_end(args);

	fputwc(L'\n', stderr);
}
