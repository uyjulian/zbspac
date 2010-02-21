/**
 * @file		ScriptUnpacker.c
 * @brief		Extracts the game's "screenplay" from the script files.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "StringUtils.h"
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
	FILE* outFile;
	if ((outFile = _wfopen(targetPath, L"wb")) == NULL) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open the target file!");
		return false;
	}

	/// The UTF16-LE BOM, UTF16-LE is the encoding used for wchar_t in Windows.
	fputs("\xFF\xFE", outFile);

	fseek(script->file, 0, SEEK_SET);
	char* data = malloc(script->fileLength);
	if (fread(data, 1, script->fileLength, script->file) != script->fileLength) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read text data from the script file!");
		free(data);
		return false;
	}

	/**
	 * Text segments are Shift-JIS encoded and separated (or more precisely,
	 * terminated) by one or more null bytes, so we can treat them just like
	 * multibyte C strings.
	 */

	encodingSwitchToShiftJIS();
	u32 index = script->textOffset;
	u32 extractedCount = 0;
	u32 ignoredCount = 0;

	while (true) {
		while (data[index] == '\0') ++index;
		/**
		 * After the text section there is a ending section that consists of
		 * 0x00, 0xFF and maybe some bytes with small value, detect them and
		 * finish the process.
		 *
		 * The type cast is used because char is unsigned on this platform,
		 * without the cast the following condition will be true for leading
		 * bytes of Japanese characters, and terminate the process too early.
		 *
		 * Now the following code behaves correctly no matter chars are
		 * signed or not.
		 */
		byte temp = (byte)(data[index]);
		if (temp < 32 || temp == 0xFF)
			break;

		u32 rawLength = strlen(data + index);

		/**
		 * If the first byte of the text segment represents an capital English
		 * letter or a digit, this segment is not regular text but special
		 * effects or name of other script files (For storyline branching).
		 * Ignore them, no need to translate.
		 */
		if (isdigit(data[index]) || isupper(data[index])) {
			index += rawLength;
			++ignoredCount;
			continue;
		}
		/**
		 * Extract the text segment and write it to the output file.
		 * The output format is designed to ease translation.
		 *
		 * For safety, we should record the length of the text segment,
		 * and then we can make sure we won't exceed the limit when
		 * editing the texts.
		 *
		 * When writing wide characters, the LF (\n) is not converted
		 * to CR LF (\r\n) automatically, as it would be when writing
		 * multibyte chars, so we have to do it manually.
		 */
		wchar_t* text = toWCString(data + index);
		fwprintf(outFile, L"%u %u\r\n%s\r\n", index, rawLength, text);
		for (int i = 0; i < rawLength; ++i) {
			fputwc('-', outFile);
		}
		fwprintf(outFile, L"\r\n%s\r\n\r\n", text);
		free(text);
		index += rawLength;
		++extractedCount;
	}

	encodingSwitchToNative();
	free(data);
	fclose(outFile);
	writeLog(LOG_NORMAL, L"%u strings extracted, %u ignored, %u total.",
			extractedCount, ignoredCount, extractedCount + ignoredCount);
	return true;
}

bool unpackScript(const wchar_t* sourcePath, const wchar_t* targetPath) {
	writeLog(LOG_NORMAL, L"Unpacking Script: %s", sourcePath);
	writeLog(LOG_NORMAL, L"To File: %s", targetPath);
	ScriptFile* script = openScriptFile(sourcePath);
	if (!script) return false;
	bool result = validateHeaderAndGetTextOffset(script)
				&& extractText(script, targetPath);
	closeScriptFile(script);
	writeLog(LOG_NORMAL, (result) ? L"Unpacking Successful." : L"ERROR: Unpacking Failed.");
	return result;
}
