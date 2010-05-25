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

static bool getNextMeaningfulLine(FILE* file, wchar_t* buf, u32 size) {
	while (fgetws(buf, CBUF_TRY_SIZE, file) != NULL) {
		u32 index = 0;
		while (buf[index] == L' ' || buf[index] == L'\t') ++index;
		if (buf[index] == L'\r' || buf[index] == L'#') continue;
		return true;
	}
	return false;
}

static bool appendFile(FILE* targetFile, wchar_t* sourcePath) {
	FILE* sourceFile = _wfopen(sourcePath, L"rb");
	if (!sourceFile) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open %s for reading!", sourcePath);
		return false;
	}

	fseek(sourceFile, 0, SEEK_END);
	int length = ftell(sourceFile);
	if (length < 0) {
		writeLog(LOG_QUIET, L"ERROR: Unable to determine the length of %s!", sourcePath);
		fclose(sourceFile);
		return false;
	}

	fseek(sourceFile, 0, SEEK_SET);
	byte* data = malloc(sizeof(byte) * length);
	if (fread(data, sizeof(byte), length, sourceFile) != length) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read %s!", sourcePath);
		fclose(sourceFile);
		return false;
	}
	fclose(sourceFile);

	if (fwrite(data, sizeof(byte), length, targetFile) != length) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write to the target file!");
		return false;
	}
	return true;
}

static bool writeTextSection(FILE* compiledScript, wchar_t* textPath) {
	FILE* plainScript = _wfopen(textPath, L"rb");
	if (!plainScript) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open %s for reading!", textPath);
		return false;
	}

	wchar_t buf[CBUF_TRY_SIZE];
	wchar_t encoding[CBUF_TRY_SIZE];
	u32 count = 0;

	/// Skip the BOM.
	fseek(plainScript, 2, SEEK_SET);

	/// Get the encoding and segment count.
	if (!getNextMeaningfulLine(plainScript, buf, CBUF_TRY_SIZE)) {
		writeLog(LOG_QUIET, L"ERROR: script.txt: header not found!");
		fclose(plainScript);
		return false;
	}

	if (swscanf(buf, L"ZBSPAC-TRANSLATION ENCODING %s COUNT %u", encoding, &count) != 2) {
		writeLog(LOG_QUIET, L"ERROR: header is corrupt");
		fclose(plainScript);
		return false;
	}

	writeLog(LOG_NORMAL, L"The script's encoding is %s, has %u strings.", encoding, count);

	for (u32 i = 0; i < count; ++i) {
		if (!getNextMeaningfulLine(plainScript, buf, CBUF_TRY_SIZE)) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read Segment %u!", i);
			fclose(plainScript);
			return false;
		}

		u32 serial, nullCount;
		if (swscanf(buf, L"SEG %u NULL %u", &serial, &nullCount) != 2) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read the serial and number of following nulls for Segment %u!", i);
			fclose(plainScript);
			return false;
		}

		if (serial != i) {
			writeLog(LOG_QUIET, L"ERROR: Segment %u's serial number is %u. They are not equal!", i, serial);
			fclose(plainScript);
			return false;
		}

		bool isText = (wcsstr(buf, L"NOT-TEXT") == NULL);

		/// This line is the original text.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);
		/// The separator.
		fgetws(buf, CBUF_TRY_SIZE, plainScript);

		/// Then the altered text.
		if (fgetws(buf, CBUF_TRY_SIZE, plainScript) == NULL) {
			writeLog(LOG_QUIET, L"ERROR: Unable to read Segment %u!", i);
			fclose(plainScript);
			return false;
		}
		/// Discard the CRLF.
		buf[wcslen(buf) - 2] = L'\0';

		char* mbText = isText
				? toMBString(buf, encoding)
				: toMBString(buf, L"japanese");

		if (mbText == NULL) {
			writeLog(LOG_QUIET, L"ERROR: Unable to convert Segment %u to the target encoding: %s", i, buf);
			return false;
		}

		u32 mbLen = strlen(mbText);
		if (fwrite(mbText, 1, mbLen, compiledScript) != mbLen) {
			writeLog(LOG_QUIET, L"ERROR: Unable to write Segment %u to the target file : %s",
				i, buf);
			free(mbText);
			return false;
		}

		for (u32 j = 0; j < nullCount; ++j) {
			fputc('\0', compiledScript);
		}
		free(mbText);
	}
	fclose(plainScript);
	return true;
}

static bool doPack(const wchar_t* sourcePath, const wchar_t* targetPath) {

	wchar_t* headPath = wcsAppend(sourcePath, L"\\head.bin");
	wchar_t* tailPath = wcsAppend(sourcePath, L"\\tail.bin");
	wchar_t* textPath = wcsAppend(sourcePath, L"\\script.txt");

	FILE* targetFile = _wfopen(targetPath, L"wb");

	if (!targetFile) {
		writeLog(LOG_QUIET, L"ERROR: Unable to open the target file for writing!");
		free(headPath); free(tailPath); free(textPath);
		return false;
	}

	/// Write the head section.
	if (!appendFile(targetFile, headPath)) {
		free(headPath); free(tailPath); free(textPath);
		fclose(targetFile);
		return false;
	}

	/// Write the text section.
	if (!writeTextSection(targetFile, textPath)) {
		free(headPath); free(tailPath); free(textPath);
		fclose(targetFile);
		return false;
	}

	/// Write the tail section.
	if (!appendFile(targetFile, tailPath)) {
		free(headPath); free(tailPath); free(textPath);
		fclose(targetFile);
		return false;
	}

	fclose(targetFile);
	return true;
}

bool packScript(const wchar_t* sourcePath, const wchar_t* targetPath) {
	writeLog(LOG_NORMAL, L"Packing Plain text script: %s", sourcePath);
	writeLog(LOG_NORMAL, L"To File: %s", targetPath);

	bool result = doPack(sourcePath, targetPath);

	writeLog(LOG_NORMAL, (result) ? L"Packing Successful." : L"ERROR: Packing Failed.");
	return result;
}
