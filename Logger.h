/**
 * @file		Logger.h
 * @brief		Provides one (indeed) very simple Logger that outputs to stderr.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include "CommonDef.h"

enum LogLevel {
	LOG_NOT_SPECIFIED,
	LOG_QUIET,
	LOG_NORMAL,
	LOG_VERBOSE
};

typedef enum LogLevel LogLevel;

void setLogLevel(LogLevel level);
void writeLog(LogLevel level, const wchar_t* str, ...);
void writeOnlyOnLevel(LogLevel level, const wchar_t* str, ...) ;

#endif
