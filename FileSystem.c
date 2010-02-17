/**
 * @file		FileSystem.c
 * @brief		Directory and path manipulation functions.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <wchar.h>
#include <direct.h>

#include "StringUtils.h"
#include "FileSystem.h"

wchar_t* fsAbsolutePath(const wchar_t* relativePath) {
	return _wfullpath(NULL, relativePath, CBUF_TRY_SIZE);
}

wchar_t* fsCombinePath(const wchar_t* directory, const wchar_t* filename) {
	u32 dirLen = wcslen(directory);
	u32 fileLen = wcslen(filename);

	u32 resLen = dirLen + 1 + fileLen;
	wchar_t* res = newWCString(resLen);

	wcscpy(res, directory);
	res[dirLen] = '\\';
	wcscpy(res + dirLen + 1, filename);
	return res;
}

bool fsEnsureDirectoryExists(const wchar_t* dir) {
	if (dir == NULL) return false;

	wchar_t* curDir = _wgetcwd(NULL, CBUF_TRY_SIZE);

	/// If the directory already exists, do nothing.
	if (_wchdir(dir) == 0) {
		_wchdir(curDir);
		free(curDir);
		return true;
	}

	/**
	 * Create all directories along the specified path.
	 */

	wchar_t* workPath = newWCString(wcslen(dir));
	wcscpy(workPath, dir);

	wchar_t* pathComponent = wcstok(workPath, L"\\");
	bool result = false;
	while (pathComponent) {
		_wmkdir(pathComponent);
		result = _wchdir(pathComponent);
		if (result != 0) break;
		pathComponent = wcstok(NULL, L"\\");
	}

	_wchdir(curDir);
	free(curDir);
	free(workPath);

	return result == 0;
}
