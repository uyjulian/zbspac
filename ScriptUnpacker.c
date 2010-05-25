/**
 * @file		ScriptUnpacker.c
 * @brief		Extracts the game's "screenplay" from the script files.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

/**
 * As of 2010.05.25, the script is unpacked into a directory,
 * just like a package would be.
 * There are three files in the directory:
 * head.bin : the data in the original binary script, before the text section
 * tail.bin : the data after the text section
 * script.txt : the text section, unpacked.
 *
 * This change has two advantages:
 *
 * 1. The original script packer needs an unmodified binary script file when
 *    packing the translated strings back, but in reality the binary may
 *    already be updated with some translations, and packing will produce
 *    a corrupted bin file. Now that we have all the original data in our
 *    hands, we are not afraid of the existing bin file being changed. In fact,
 *    we now need no bin file at all, once we have extracted the texts.
 *
 * 2. The PAC package generator skips subdirectoies in the source dir, so now
 *    the unpacked scripts will not be packed back into the package even if
 *    they are in the source dir. And we like defaults, don't we? ;)
 */

/**
 * As of 2010.05.23, the output format is changed.
 * I suspect that the game doesn't have pointers to individual script segments,
 * rather it reads the text section sequentially, but the numbers of nulls
 * between two segments are fixed, changing them would crash the game.
 * So for the text section as a whole, now I will just store its beginning and
 * end, then for each segment, I record the number of spaces following it.
 */

#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "StringUtils.h"
#include "Filesystem.h"
#include "ScriptFile.h"

struct ScriptFile {
	u64 textOffset;
	u32 fileLength;
	FILE* file;
};
typedef struct ScriptFile ScriptFile;

static void closeScriptFile(ScriptFile* script) {
	if (script == NULL) return;
	if (script->file != NULL) fclose(script->file);
	free(script);
	script = NULL;
}

static ScriptFile* openScriptFile(const wchar_t* sourcePath) {
	ScriptFile* script = malloc(sizeof(ScriptFile));
	memset(script, 0, sizeof(ScriptFile));

	if (!(script->file = _wfopen(sourcePath, L"rb"))) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open the script file.");
		closeScriptFile(script);
		return NULL;
	}
	writeLog(LOG_VERBOSE, L"Script File Opened.");
	return script;
}

static bool validateHeaderAndGetTextOffset(ScriptFile* script) {
	/**
	 * The first 8 bytes of the script file is an unsigned integer n,
	 * and (n + 1) * 8 is the offset of the text section. (The text
	 * section may have some leading null bytes with a length smaller
	 * than 4).
	 * From the patterns of the data before the text section, we may
	 * assume that the data is an array of '8-byte-long data entries',
	 * and 'n' is indeed the size of this array.
	 * It's not clear what those entries actually are, (typically they
	 * would be some kind of pointers into the text) but they don't
	 * seem to matter as long as we only mess with the text itself.
	 *
	 * In my implementation, the textOffset member of ScriptFile consumes
	 * 8 bytes, but actually it won't exceed u32's range. The use of a u64
	 * here is for simplifying the file type validation : There is now no
	 * need to deal with Byte 4 to 7 separately.
	 */

	if (fread(&(script->textOffset), 8, 1, script->file) != 1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the text offset from the script!");
		return false;
	}

	script->textOffset = (script->textOffset + 1) * 8;

	fseek(script->file, 0, SEEK_END);
	script->fileLength = ftell(script->file);
	if (script->fileLength == -1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to get the length of the script!");
		return false;
	}

	if (script->textOffset > script->fileLength) {
		writeLog(LOG_QUIET, L"ERROR: The source file is not a vaild script file!");
		return false;
	}
	writeLog(LOG_VERBOSE, L"The source file's length is %u, and the text begins at %u",
			script->fileLength, script->textOffset);
	return true;
}

static bool extractText(ScriptFile* script, const wchar_t* targetPath) {

	if (!fsEnsureDirectoryExists(targetPath)) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open or create the target directory!");
		return false;
	}

	wchar_t* headPath = wcsAppend(targetPath, L"\\head.bin");
	wchar_t* tailPath = wcsAppend(targetPath, L"\\tail.bin");
	wchar_t* textPath = wcsAppend(targetPath, L"\\script.txt");

	fseek(script->file, 0, SEEK_SET);
	char* data = malloc(script->fileLength);
	if (fread(data, 1, script->fileLength, script->file) != script->fileLength) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read text data from the script file!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		return false;
	}

	/**
	 * Put the head section into head.bin.
	 * We need to know where the text truly starts.
	 * (There may be some nulls before the text segments.)
	 */
	u32 index = script->textOffset;
	while (data[index] == '\0') ++index;

	FILE* headFile;
	if ((headFile = _wfopen(headPath, L"wb")) == NULL) {
		writeLog(LOG_QUIET, L"ERROR: Unable to create head.bin!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		return false;
	}

	if (fwrite(data, sizeof(byte), index, headFile) != index) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write to head.bin!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		fclose(headFile);
		return false;
	}
	fclose(headFile);

	/**
	 * Now extract the texts.
	 * Somehow opening the text file in text mode results in a mess after
	 * texts are written. So I use binary mode here.
	 * And remember to output '\r\n' when writing newlines.
	 */

	FILE* textFile;
	if ((textFile = _wfopen(textPath, L"wb")) == NULL) {
		writeLog(LOG_QUIET, L"ERROR: Unable to create script.txt!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		return false;
	}

	/// The UTF16-LE BOM, UTF16-LE is the encoding used for wchar_t in Windows.
	fputs("\xFF\xFE", textFile);
	/// Header
	fwprintf(textFile, L"ZBSPAC-TRANSLATION ENCODING japanese COUNT      \r\n\r\n");

	/**
	 * Text segments are Shift-JIS encoded and separated (or more precisely,
	 * terminated) by one or more null bytes, so we can treat them just like
	 * multibyte C strings.
	 */

	u32 extractedCount = 0;
	u32 textCount = 0;

	while (true) {
		/**
		 * After the text section there is a ending section that consists of
		 * 0x00, 0xFF and maybe some bytes with small value, detect them and
		 * store its starting offset, so the packer can perserve this section
		 * in the recompiled script.
		 *
		 * The type cast is used because char is unsigned on this platform,
		 * without the cast the following condition will be true for leading
		 * bytes of Japanese characters, and terminate the process too early.
		 *
		 * Now the following code behaves correctly no matter chars are
		 * signed or not.
		 */
		byte temp = (byte)(data[index]);
		if (temp < 32 || temp == 0xFF) {
			break;
		}

		/**
		 * Extract the text segment and write it to the output file.
		 * The output format is designed to ease translation.
		 *
		 * Now there is no need to restrict the texts to fit in a
		 * given length, but we need to know how many nulls follow.
		 *
		 * If the first byte of the text segment represents an English letter
		 * or a digit, or this file's name ends in '.bin', then this segment is
		 * not regular text but special effects or name of other script files
		 * (For storyline branching).
		 * DO NOT ignore them, as they have their places in the script.
		 * Mark them as 'NOT-TEXT'.
		 */

		bool notText = isdigit(data[index]) || isupper(data[index]);

		wchar_t* text = toWCString(data + index, L"japanese");
		u32 textLen = wcslen(text);

		if (textLen > 4 && wcscmp(text + textLen - 4, L".bin") == 0) notText = true;
		u32 rawLength = strlen(data + index);
		index += rawLength;

		u32 followingNulls = 0;
		while (data[index] == '\0') {
			++followingNulls;
			++index;
		}

		fwprintf(textFile, L"SEG %u NULL %u %s\r\n",
				extractedCount, followingNulls,
				notText ? L"NOT-TEXT" : L"");
		fwprintf(textFile, L"%s\r\n", text);

		for (int i = 0; i < rawLength; ++i) {
			fputwc('-', textFile);
		}
		fwprintf(textFile, L"\r\n%s\r\n\r\n", text);
		free(text);
		++extractedCount;
		if (!notText) ++textCount;
	}

	/// This position is in the header, just after "COUNT"
	fseek(textFile, 88, SEEK_SET);
	fwprintf(textFile, L"%5u", extractedCount);
	fclose(textFile);

	/// Now store the tail part.
	FILE* tailFile;
	if ((tailFile = _wfopen(tailPath, L"wb")) == NULL) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open tail.bin!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		return false;
	}

	u32 tailLen = script->fileLength - index;
	if (fwrite(data + index, sizeof(byte), tailLen, tailFile) != tailLen) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write to tail.bin!");
		free(headPath); free(tailPath); free(textPath);
		free(data);
		fclose(tailFile);
		return false;
	}
	fclose(tailFile);

	free(headPath); free(tailPath); free(textPath);
	free(data);

	writeLog(LOG_NORMAL, L"%u strings translatable, %u not, %u total.",
			textCount, extractedCount - textCount, extractedCount);
	return true;
}

bool unpackScript(const wchar_t* sourcePath, const wchar_t* targetPath) {
	writeLog(LOG_NORMAL, L"Unpacking Script: %s", sourcePath);
	writeLog(LOG_NORMAL, L"To Directory: %s", targetPath);
	ScriptFile* script = openScriptFile(sourcePath);
	if (!script) return false;
	bool result = validateHeaderAndGetTextOffset(script)
				&& extractText(script, targetPath);
	closeScriptFile(script);
	writeLog(LOG_NORMAL, (result) ? L"Unpacking Successful." : L"ERROR: Unpacking Failed.");
	return result;
}
