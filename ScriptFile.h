/**
 * @file		ScriptFile.h
 * @brief		Declares functions the manipulate compiled scripts (*.bin).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */
#ifndef COMPILED_SCRIPT_FILE_H_INCLUDED
#define COMPILED_SCRIPT_FILE_H_INCLUDED

#include "CommonDef.h"

bool unpackScript(const wchar_t* sourcePath, const wchar_t* targetPath);
bool packScript(const wchar_t* sourcePath, const wchar_t* targetPath);

#endif /* COMPILEDSCRIPTFILE_H_ */
