/**
 * @file		ScriptPacker.c
 * @brief		Packs plain texts back into compiled scripts.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.03
 */

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <string.h>

#include "Logger.h"
#include "StringUtils.h"
#include "ScriptFile.h"

bool doPack(const wchar_t* sourcePath, const wchar_t* targetPath) {
	/// The translated script should be encoded in the native character set.
	encodingSwitchToNative();

	FILE* plainScript = _wfopen(sourcePath, L"rb");
	FILE* compiledScript = _wfopen(targetPath, L"r+");

	if (!plainScript || !compiledScript) {
		writeLog(LOG_QUIET, L"ERROR: Cannot open the source or the target files!");
		fclose(plainScript);
		fclose(compiledScript);
		return false;
	}

	u32 offset = 0;
	u32 maxLen = 0;
	u32 index = 0;
	wchar_t buf[CBUF_TRY_SIZE];

	/// Skip the BOM.
	fseek(plainScript, 2, SEEK_SET);

	while (true) {
		/// This line includes offset and maximum length
		if (fgetws(buf, CBUF_TRY_SIZE, plainScript) == NULL)
			break;

		if (swscanf(buf, L"%u %u", &offset, &maxLen) != 2) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read the offset and maximum length for Entry %u!", index);
			fclose(plainScript);
			fclose(compiledScript);
			return false;;
		}
		/// This line is the original text.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);
		/// The separator.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);

		/// Then the altered text.
		if (fgetws(buf, CBUF_TRY_SIZE, plainScript) == NULL) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read the next segment! \n  Entry: %u, Offset: %u, Maximum length: %u",
								index, offset, maxLen);
			fclose(plainScript);
			fclose(compiledScript);
			return false;
		}

		/// Discard the CRLF.
		buf[wcslen(buf) - 2] = L'\0';

		char* mbText = toMBString(buf);

		i32 mbLen = strlen(mbText);
		i32 paddingLen = maxLen - mbLen;
		if (paddingLen < 0) {
			writeLog(LOG_QUIET, L"ERROR: Segment Too Long! \n  Entry: %u, Offset: %u, Maximum: %u, Actual: %u, Value: %s",
								index, offset, maxLen, mbLen, buf);
			fclose(plainScript);
			fclose(compiledScript);
			free(mbText);
			return false;
		}

		if (fseek(compiledScript, offset, SEEK_SET) != 0) {
			writeLog(LOG_QUIET, L"ERROR: Unable to locate the injection point for next string segment! \n  Entry: %u, Offset: %u, Maximum: %u, Actual: %u, Value: %s",
						index, offset, maxLen, mbLen, buf);
			fclose(plainScript);
			fclose(compiledScript);
			free(mbText);
			return false;
		}


		if (fwrite(mbText, 1, mbLen, compiledScript) != mbLen) {
			writeLog(LOG_QUIET, L"ERROR: Cannot write the segment to the target file! \n  Entry: %u, Offset: %u, Maximum: %u, Actual: %u, Value: %s",
				index, offset, maxLen, mbLen, buf);
			fclose(plainScript);
			fclose(compiledScript);
			free(mbText);
			return false;
		}

		/**
		 * Padding with the null character will crash the game,
		 * so we use whitespace instead.
		 */
		for (i32 i = 0; i < paddingLen; ++i) {
			fwrite(" ", 1, 1, compiledScript);
		}

		writeLog(LOG_VERBOSE, L"Written: Entry: %u, Offset: %u, Maximum: %u, Actual: %u, Value: %s",
			index, offset, maxLen, mbLen, buf);

		/// Consume the blank line after each entry.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);

		++index;
		free(mbText);
	}

	writeLog(LOG_NORMAL, L"%u string segments packed.", index);
	fclose(plainScript);
	fclose(compiledScript);
	return true;
}

bool packScript(const wchar_t* sourcePath, const wchar_t* targetPath) {
	writeLog(LOG_NORMAL, L"Packing Plain text script: %s", sourcePath);
	writeLog(LOG_NORMAL, L"To File: %s", targetPath);

	bool result = doPack(sourcePath, targetPath);

	writeLog(LOG_NORMAL, (result) ? L"Packing Successful." : L"ERROR: Packing Failed.");
	return result;
}
