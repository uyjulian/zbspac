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
	wchar_t* targetDir = fsAbsolutePath(dir);
	wchar_t* currentDir = _wgetcwd(NULL, CBUF_TRY_SIZE);

	/// If the directory already exists, do nothing.
	if (_wchdir(targetDir) == 0) {
		_wchdir(currentDir);
		free(currentDir);
		return true;
	}

	/**
	 * Create all directories along the specified path.
	 */
	wchar_t* workPath = newWCString(wcslen(dir));
	wcscpy(workPath, dir);

	/**
	 * The first component of the path is the drive specifier.
	 * We should go to its root first.
	 */
	wchar_t* pathComponent = wcstok(workPath, L"\\");
	wchar_t* initialPath = wcsAppend(pathComponent, L"\\");
	_wchdir(initialPath);
	free(initialPath);
	pathComponent = wcstok(NULL, L"\\");

	bool status = 0;
	while (pathComponent) {
		_wmkdir(pathComponent);
		status = _wchdir(pathComponent);
		if (status != 0) break;
		pathComponent = wcstok(NULL, L"\\");
	}

	_wchdir(currentDir);
	free(currentDir);
	free(workPath);

	return status == 0;
}
