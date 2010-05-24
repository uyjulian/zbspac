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

struct SegmentData {
	wchar_t* text;
	bool isText;
	u32 nullCount;
};

typedef struct SegmentData SegmentData;

static bool getNextMeaningfulLine(FILE* file, wchar_t* buf, u32 size) {
	while (fgetws(buf, CBUF_TRY_SIZE, file) != NULL) {
		u32 index = 0;
		while (buf[index] == L' ' || buf[index] == L'\t') ++index;
		if (buf[index] == L'\r' || buf[index] == L'#') continue;
		return true;
	}
	return false;
}

bool doPack(const wchar_t* sourcePath, const wchar_t* targetPath) {

	FILE* plainScript = _wfopen(sourcePath, L"rb");
	FILE* compiledScript = _wfopen(targetPath, L"rb");

	if (!plainScript || !compiledScript) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open needed files!");
		fclose(plainScript);
		fclose(compiledScript);
		return false;
	}

	/**
	 * This file is opened only for existence checking.
	 * Close it now, and reopen it when the source file is successfully read.
	 */
	fclose(compiledScript);

	wchar_t buf[CBUF_TRY_SIZE];
	wchar_t encoding[CBUF_TRY_SIZE];
	u32 start, end, count;

	/// Skip the BOM.
	fseek(plainScript, 2, SEEK_SET);

	if (!getNextMeaningfulLine(plainScript, buf, CBUF_TRY_SIZE)) {
		writeLog(LOG_QUIET, L"ERROR: Script properties not found!");
		fclose(plainScript);
		return false;
	}

	if (swscanf(buf, L"ZBSPAC-TRANSLATION %s %u %u %u", encoding, &start, &end, &count) != 4) {
		writeLog(LOG_QUIET, L"ERROR: Unable to load script properties!");
		fclose(plainScript);
		return false;
	}
	writeLog(LOG_VERBOSE, L"Encoding: %s, Start: %u, End: %u, Count: %u", encoding, start, end, count);

	/// Now the reading begins.
	SegmentData* data = malloc (sizeof(SegmentData) * count);
	memset(data, 0, sizeof(SegmentData) * count);

	for (u32 i = 0; i < count; ++i) {
		u32 serial, nullCount;

		if (!getNextMeaningfulLine(plainScript, buf, CBUF_TRY_SIZE)) {
			writeLog(LOG_QUIET, L"ERROR: Unable to locate the next data section!");
			fclose(plainScript);
			free(data);
			return false;
		}

		if (swscanf(buf, L"SEG %u NULL %u", &serial, &nullCount) != 2) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read the serial and number of following nulls for Segment %u!", i);
			fclose(plainScript);
			free(data);
			return false;
		}

		if (serial != i) {
			writeLog(LOG_QUIET, L"ERROR: Segment %u's serial number is %u. They are not equal!", i, serial);
			fclose(plainScript);
			free(data);
			return false;
		}

		data[i].isText = (wcsstr(buf, L"NOT-TEXT") == NULL);
		data[i].nullCount = nullCount;

		/// This line is the original text.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);
		/// The separator.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);

		/// Then the altered text.
		if (fgetws(buf, CBUF_TRY_SIZE, plainScript) == NULL) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read Segment %u!", i);
			fclose(plainScript);
			free(data);
			return false;
		}
		/// Discard the CRLF.
		buf[wcslen(buf) - 2] = L'\0';

		data[i].text = cloneWCString(buf);

		// Consume the blank line following.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);
	}
	fclose(plainScript);

	// Then let's store the tail part of the binary script.
	compiledScript = _wfopen(targetPath, L"rb+");
	if (!plainScript || !compiledScript) {
		writeLog(LOG_QUIET, L"ERROR: Cannot open the target file!");
		fclose(compiledScript);
		free(data);
		return false;
	}
	fseek(compiledScript, -1, SEEK_END);
	u32 tailLen = ftell(compiledScript) - end + 1;
	byte* tailData = malloc(sizeof(byte) * tailLen);
	fseek(compiledScript, end, SEEK_SET);
	if (fread(tailData, sizeof(byte), tailLen, compiledScript) != tailLen) {
		writeLog(LOG_QUIET, L"ERROR: Cannot read the tail part of the target file!");
		fclose(compiledScript);
		free(tailData);
		return false;
	}


	// Now begin to write.
	fseek(compiledScript, start, SEEK_SET);

	for (u32 i = 0; i < count; ++i) {
		char* mbText = data[i].isText
				? toMBString(data[i].text, encoding)
				: toMBString(data[i].text, L"japanese");

		if (mbText == NULL) {
			writeLog(LOG_QUIET, L"ERROR: Unable to convert Segment %u to the target encoding: %s", i, buf);
			fclose(compiledScript);
			free(tailData);
			return false;
		}

		u32 mbLen = strlen(mbText);
		if (fwrite(mbText, 1, mbLen, compiledScript) != mbLen) {
			writeLog(LOG_QUIET, L"ERROR: Unable to write Segment %u to the target file : %s",
				i, data[i].text);
			fclose(compiledScript);
			free(tailData);
			free(mbText);
			return false;
		}

		for (u32 j = 0; j < data[i].nullCount; ++j) {
			fputc('\0', compiledScript);
		}
		free(mbText);
	}

	// Write the original tail part back.
	if (fwrite(tailData, sizeof(byte), tailLen, compiledScript) != tailLen) {
		writeLog(LOG_QUIET, L"ERROR: Cannot write the tail part to the target file!");
		fclose(compiledScript);
		free(tailData);
		return false;
	}

	writeLog(LOG_NORMAL, L"%u string segments packed.", count);
	fclose(compiledScript);
	free(tailData);
	return true;
}

bool packScript(const wchar_t* sourcePath, const wchar_t* targetPath) {
	writeLog(LOG_NORMAL, L"Packing Plain text script: %s", sourcePath);
	writeLog(LOG_NORMAL, L"To File: %s", targetPath);

	bool result = doPack(sourcePath, targetPath);

	writeLog(LOG_NORMAL, (result) ? L"Packing Successful." : L"ERROR: Packing Failed.");
	return result;
}
