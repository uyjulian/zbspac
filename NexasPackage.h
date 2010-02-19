/**
 * @file		NexasPackage.h
 * @brief		This file defines functions manipulating the PAC files.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef NEXAS_PACKAGE_H_INCLUDED
#define NEXAS_PACKAGE_H_INCLUDED

#include "CommonDef.h"

bool unpackPackage(const wchar_t* packagePath, const wchar_t* targetDir);
bool packPackage(const wchar_t* sourceDir, const wchar_t* packagePath, bool shouldZip);

#endif
