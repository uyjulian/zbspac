/**
 * @file		zbspac.c
 * @brief		PAC resource file extractor / packager for Baldr Sky Dive 1 & 2.
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

const wchar_t* USAGE_STRING = L"Usage: zbspac [quietly|verbosely] <operation> source_path [target_path]";

void init() {
	setLogLevel(LOG_NORMAL);
	encodingSwitchToNative();
}

bool processPackCmd(CmdArgs* args) {
	return packPackage(argSourcePath(args), argTargetPath(args));
}

bool processUnpackCmd(CmdArgs* args) {
	return unpackPackage(argSourcePath(args), argTargetPath(args));
}


bool processAboutCmd(CmdArgs* args) {
	writeOnlyOnLevel(LOG_QUIET, L"Shhhhhhh...... I should stay quiet......");
	writeLog(LOG_NORMAL, L"zbspac: CloudiDust's resource (un)packer for Baldr Sky.");
	writeLog(LOG_NORMAL, L"Copyright 2010, CloudiDust. Covered by 2-clause BSD license.");
	writeLog(LOG_NORMAL, L"Special thanks to 痴汉公贼(jzhang0) & asmodean.");
	return true;
}

bool processHelpCmd(CmdArgs* args) {
	writeOnlyOnLevel(LOG_QUIET, L"Shhhhhhh...... I should stay quiet......");
	writeLog(LOG_NORMAL, L"Usage: zbspac [quietly|verbosely] <operation> source_path [target_path]");
	writeLog(LOG_NORMAL,L"");
	writeLog(LOG_NORMAL,L"  You should specify the operation you want to perform:");
	writeLog(LOG_NORMAL,L"");
	writeLog(LOG_NORMAL,L"    pack   -- pack all files under a given directory into a package.");
	writeLog(LOG_NORMAL,L"    unpack -- unpack a package and place the contents in a directory.");
	writeLog(LOG_NORMAL,L"    help   -- display this page. (Maybe)");
	writeLog(LOG_NORMAL,L"    about  -- display some copyright information.");
	writeLog(LOG_NORMAL,L"");
	writeLog(LOG_NORMAL,L"  You may define how noisy this program will be,");
	writeLog(LOG_NORMAL,L"  just add 'quietly' or 'verbosely' before the operation.");
	writeLog(LOG_NORMAL,L"  In 'quiet' mode, nothing will be displayed if everything goes on well.");
	writeLog(LOG_NORMAL,L"  And 'verbose' mode is mainly for debug purposes.");
	writeLog(LOG_NORMAL,L"");
	writeLog(LOG_NORMAL,L"  If no target is specified, a default path will be used.");
	writeLog(LOG_NORMAL,L"  For packing, it is the source path with '.pac' suffix.");
	writeLog(LOG_NORMAL,L"  For unpacking, it is the source path without extension.");
	writeLog(LOG_NORMAL,L"  If the source package name itself has no extension,");
	writeLog(LOG_NORMAL,L"  a '_' suffix will be appended to avoid name collision.");
	writeLog(LOG_NORMAL,L"");
	writeLog(LOG_NORMAL,L"  When packing a new package, subdirectories are ignored,");
	writeLog(LOG_NORMAL,L"  and the file names should not exceed 63 bytes.");
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
	case CMD_UNPACK:
		result = processUnpackCmd(args);
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
