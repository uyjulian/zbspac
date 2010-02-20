/**
 * @file		CmdArgs.c
 * @brief		Command line parser and object for arguments.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "StringUtils.h"
#include "FileSystem.h"
#include "CmdArgs.h"

struct CmdArgs {
	CmdType cmdType;
	LogLevel logLevel;
	wchar_t* sourcePath;
	wchar_t* targetPath;
};

/**
 * The parser is a simple FSM, that accepts:
 * (quietly|verbosely)? (pack|zip|unpack|help|about) (source_path) (target_path)?
 */

enum StateCode {
	APS_WAITING_CMD_OR_LOG_LEVEL,
	APS_WAITING_CMD,
	APS_WAITING_SOURCE,
	APS_WAITING_TARGET,
	APS_FINISHED,
	APS_ERROR
};
typedef enum StateCode StateCode;

void deleteCmdArgs(CmdArgs* args) {
	if (args == NULL) return;
	if (args->sourcePath != NULL) {
		free(args->sourcePath);
		args->sourcePath = NULL;
	}
	if (args->targetPath != NULL) {
		free(args->targetPath);
		args->targetPath = NULL;
	}
	free(args);
	args = NULL;
}

static StateCode readCmd(CmdArgs* args, const char* str) {
	if (strcmp(str, "pack") == 0) {
		args->cmdType = CMD_PACK;
		return APS_WAITING_SOURCE;
	}
	if (strcmp(str, "zip") == 0) {
			args->cmdType = CMD_ZIP;
			return APS_WAITING_SOURCE;
	}
	if (strcmp(str, "unpack") == 0) {
		args->cmdType = CMD_UNPACK;
		return APS_WAITING_SOURCE;
	}
	if (strcmp(str, "help") == 0) {
		args->cmdType = CMD_HELP;
		return APS_FINISHED;
	}
	if (strcmp(str, "about") == 0) {
			args->cmdType = CMD_ABOUT;
			return APS_FINISHED;
	}
	return APS_ERROR;
}

static StateCode readCmdOrLogLevel(CmdArgs* args, const char* str) {
	if (strcmp(str, "verbosely") == 0) {
		args->logLevel = LOG_VERBOSE;
		return APS_WAITING_CMD;
	}
	if (strcmp(str, "quietly") == 0) {
		args->logLevel = LOG_QUIET;
		return APS_WAITING_CMD;
	}

	return readCmd(args, str);
}

static StateCode readSourcePath(CmdArgs* args, const char* str) {
	if (str == NULL)
		return APS_ERROR;

	args->sourcePath = toWCString(str);
	return APS_WAITING_TARGET;
}

static StateCode readTargetPath(CmdArgs* args, const char* str) {
	if (str == NULL)
		/// We will use the default target path.
		return APS_FINISHED;

	args->targetPath = toWCString(str);
	return APS_FINISHED;
}

static void useAbsolutePath(CmdArgs* args) {
	wchar_t* aSourcePath = fsAbsolutePath(args->sourcePath);
	free(args->sourcePath);
	args->sourcePath = aSourcePath;

	if (args->targetPath != NULL) {
		wchar_t* aTargetPath = fsAbsolutePath(args->targetPath);
		free(args->targetPath);
		args->targetPath = aTargetPath;
	}
}

static void fillWithDefaultArgs(CmdArgs* args) {
	if (args->logLevel == LOG_NOT_SPECIFIED)
		args->logLevel = LOG_NORMAL;

	if (args->targetPath == NULL) {
		if (args->cmdType == CMD_PACK || args->cmdType == CMD_ZIP) {
			/**
			 * Target should be a package file.
			 * To obtain the default path, append '.pac' to source path,
			 */
			args->targetPath = wcsAppend(args->sourcePath, L".pac");
		} else if (args->cmdType == CMD_UNPACK) {
			/**
			 * Target should be a directory.
			 * To obtain the default path, remove the extension.
			 * Be aware that the last dot may not be a indicator of extension,
			 * e.g. c:\abc.def\some_file, so we need to know who appears first,
			 * the last dot, or the last backslash.
			 * When dealing with an extension-less package name, we append
			 * "_" to obtain the default path.
			 */
			i32 lastDotLoc = wcsFindChar(args->sourcePath, L'.', false);
			i32 lastBackslashLoc = wcsFindChar(args->sourcePath, L'\\', false);

			if (lastDotLoc > lastBackslashLoc) {
				// The above already implied lastDotLoc != -1
				// Remove the extension.
				args->targetPath = wcsSubstring(args->sourcePath, 0, lastDotLoc);
			} else {
				args->targetPath = wcsAppend(args->sourcePath, L"_");
			}
		}
	}
}

CmdArgs* parseCmdLine(int argc, char** argv) {
	if (argc == 1) return NULL;

	CmdArgs* args = malloc(sizeof(CmdArgs));
	memset(args, 0, sizeof(CmdArgs));

	u8 index = 1;
	StateCode state = APS_WAITING_CMD_OR_LOG_LEVEL;

	while (state != APS_FINISHED && state != APS_ERROR) {
		const char* currStr =
				(index < argc) ? argv[index] : NULL;

		switch (state) {
		case APS_WAITING_CMD_OR_LOG_LEVEL:
			state = readCmdOrLogLevel(args, currStr);
			break;
		case APS_WAITING_CMD:
			state = readCmd(args, currStr);
			break;
		case APS_WAITING_SOURCE:
			state = readSourcePath(args, currStr);
			break;
		case APS_WAITING_TARGET:
			state = readTargetPath(args, currStr);
			break;
		default:
			break;
		}
		++index;
	}

	/// All arguments should have been read.
	if (index < argc) state = APS_ERROR;

	if (state == APS_FINISHED) {
		useAbsolutePath(args);
		fillWithDefaultArgs(args);
		return args;
	}

	deleteCmdArgs(args);
	return NULL;
}

CmdType argCmdType(const CmdArgs* args) {
	return args->cmdType;
}

const wchar_t* argSourcePath(const CmdArgs* args) {
	return args->sourcePath;
}

const wchar_t* argTargetPath(const CmdArgs* args) {
	return args->targetPath;
}
const LogLevel argLogLevel(const CmdArgs* args) {
	return args->logLevel;
}
