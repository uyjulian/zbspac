/**
 * @file		zbspac.c
 * @brief		PAC resource file extractor / packager for Baldr Sky & Force EXE.
 * 				This is the command line user interface module.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 * @warning		This utility is specially designed for the resource file format
 *				used in Baldr Sky, namely PAC format for GIGA's NeXaS engine,
 *				Variant 4, and may be incompatible with other GIGA games.
 */

#include <stdlib.h>

#include "Logger.h"
#include "StringUtils.h"
#include "CmdArgs.h"
#include "NexasPackage.h"
#include "ScriptFile.h"

const wchar_t* USAGE_STRING = L"Usage: zbspac [quietly|verbosely] <operation> source_path [target_path]";

void init() {
	setLogLevel(LOG_NORMAL);
	encodingSwitchToNative();
}

bool processPackCmd(CmdArgs* args) {
	return packPackage(argSourcePath(args), argTargetPath(args), false);
}

bool processPackBfeCmd(CmdArgs* args) {
	return packPackage(argSourcePath(args), argTargetPath(args), true);
}

bool processUnpackCmd(CmdArgs* args) {
	return unpackPackage(argSourcePath(args), argTargetPath(args));
}

bool processPackScriptCmd(CmdArgs* args) {
	return packScript(argSourcePath(args), argTargetPath(args));
}

bool processUnpackScriptCmd(CmdArgs* args) {
	return unpackScript(argSourcePath(args), argTargetPath(args));
}

bool processAboutCmd(CmdArgs* args) {
	writeOnlyOnLevel(LOG_QUIET, L"Shhhhhhh...... I should stay quiet......");
	writeLog(LOG_NORMAL, L"zbspac: a resource (un)packer for Baldr Sky / Baldr Force EXE.");
	writeLog(LOG_NORMAL, L"Copyright 2010, CloudiDust.");
	writeLog(LOG_NORMAL, L"Special thanks to 痴汉公贼(jzhang0) & asmodean.");
	return true;
}

bool processHelpCmd(CmdArgs* args) {
	writeOnlyOnLevel(LOG_QUIET, L"Shhhhhhh...... I should stay quiet......");
	writeLog(LOG_NORMAL, L"Usage: zbspac [quietly|verbosely] <operation> source_path [target_path]");
	writeLog(LOG_NORMAL, L"");
	writeLog(LOG_NORMAL, L"Available operations are:");
	writeLog(LOG_NORMAL, L"  pack, pack-bfe, unpack, pack-script, unpack-script, help, about");
	writeLog(LOG_NORMAL, L"");
	writeLog(LOG_NORMAL, L"Please refer to instructions.txt for detail.");

	return true;
}

int main(int argc, char** argv) {
	init();

	CmdArgs* args = parseCmdLine(argc, argv);
	if (!args) {
		writeLog(LOG_QUIET, L"Oops, invalid or not enough arguments. Try 'zbspac help' for command syntax.");
		deleteCmdArgs(args);
		return EXIT_FAILURE;
	}

	setLogLevel(argLogLevel(args));
	bool result;

	switch (argCmdType(args)) {
	case CMD_PACK:
		result = processPackCmd(args);
		break;
	case CMD_PACK_BFE:
		result = processPackBfeCmd(args);
		break;
	case CMD_UNPACK:
		result = processUnpackCmd(args);
		break;
	case CMD_PACK_SCRIPT:
		result = processPackScriptCmd(args);
	break;
	case CMD_UNPACK_SCRIPT:
		result = processUnpackScriptCmd(args);
		break;
	case CMD_ABOUT:
		result = processAboutCmd(args);
		break;
	case CMD_HELP:
	default:
		result = processHelpCmd(args);
		break;
	}
	deleteCmdArgs(args);

	return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
