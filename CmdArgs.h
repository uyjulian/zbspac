/**
 * @file		CmdArgs.h
 * @brief		Command line parser and arguments object.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#ifndef CMD_ARGS_H_INCLUDED
#define CMD_ARGS_H_INCLUDED

#include "CommonDef.h"

struct CmdArgs;
typedef struct CmdArgs CmdArgs;

enum CmdType {
	CMD_PACK,
	CMD_PACK_BFE,
	CMD_UNPACK,
	CMD_PACK_SCRIPT,
	CMD_UNPACK_SCRIPT,
	CMD_HELP,
	CMD_ABOUT
};
typedef enum CmdType CmdType;

CmdArgs* parseCmdLine(int argc, char** argv);
void deleteCmdArgs(CmdArgs* args);

CmdType argCmdType(const CmdArgs* args);
const wchar_t* argSourcePath(const CmdArgs* args);
const wchar_t* argTargetPath(const CmdArgs* args);
const LogLevel argLogLevel(const CmdArgs* args);

#endif
