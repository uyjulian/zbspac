/**
 * @file		NexasUnpacker.c
 * @brief		Implementation of the unpacker.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "Logger.h"
#include "StringUtils.h"
#include "FileSystem.h"
#include "HuffmanCode.h"
#include "NexasPackage.h"

enum VariantType {
	CONTENT_NOT_COMPRESSED,
	CONTENT_LZSS,
	CONTENT_HUFFMAN,
	CONTENT_DEFLATE,
	CONTENT_MAYBE_DEFLATE
};

struct Header {
	char typeTag[3];
	byte magicByte;
	u32 entryCount;
	u32 variantTag;
};
typedef struct Header Header;

struct IndexEntry {
	char name[64];
	u32 offset;
	u32 decodedLen;
	u32 encodedLen;
};
typedef struct IndexEntry IndexEntry;

struct NexasPackage {
	Header* header;
	ByteArray* indexes;
	FILE* file;
};
typedef struct NexasPackage NexasPackage;

static void closePackage(NexasPackage* package) {
	if (!package) return;
	if (package->header)
		free(package->header);
	if (package->file) {
		fclose(package->file);
		package->file = NULL;
	}
	if (package->indexes)
		deleteByteArray(package->indexes);
	free(package);
	package = NULL;
}

static NexasPackage* openPackage(const wchar_t* packagePath) {
	NexasPackage* package = malloc(sizeof(NexasPackage));
	memset(package, 0, sizeof(NexasPackage));

	if (!(package->file = _wfopen(packagePath, L"rb"))) {
		writeLog(LOG_QUIET, L"ERROR: Cannot open the package file: %s.", packagePath);
		closePackage(package);
		return NULL;
	}
	writeLog(LOG_VERBOSE, L"Target file Opened.");
	return package;
}

static bool validateHeader(NexasPackage* package) {
	package->header = malloc(sizeof(Header));
	if (fread(package->header, sizeof(Header), 1, package->file) != 1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the package header.");
		return false;
	}

	/**
	 *  The typeTag is not null-terminated, so we cannot use strcmp here.
	 */
	if (memcmp(package->header->typeTag, "PAC", 3) != 0) {
		writeLog(LOG_QUIET, L"ERROR: Target file is not a valid package.");
		return false;
	}

	u32 vtag = package->header->variantTag;
	writeLog(LOG_VERBOSE, L"File variant tag is %d.", vtag);

	if (vtag != CONTENT_MAYBE_DEFLATE) {
		writeLog(LOG_QUIET, L"ERROR: This PAC variant is not supported yet.");
		return false;
	}
	writeLog(LOG_VERBOSE, L"Entry count: %u.", package->header->entryCount);
	return true;
}

static bool decodeIndex(NexasPackage* package) {
	fseek(package->file, -4, SEEK_END);
	u32 encodedLen;
	if (fread(&encodedLen, sizeof(u32), 1, package->file) != 1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the length of the encoded index!");
		return false;
	}
	writeLog(LOG_VERBOSE, L"The length of the compressed index is %d.", encodedLen);

	if (fseek(package->file, -4-encodedLen, SEEK_END) != 0) {
		writeLog(LOG_QUIET, L"ERROR: Unable to locate the compressed index!");
		return false;
	}

	ByteArray* encodedData = newByteArray(encodedLen);
	byte* data = baData(encodedData);
	if (fread(data, sizeof(byte), encodedLen, package->file) != encodedLen) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the compressed index!");
		deleteByteArray(encodedData);
		return false;
	}

	for (u32 i = 0; i < encodedLen; ++i) {
		data[i] ^= 0xFF;
	}

	u32 decodedLen = sizeof(IndexEntry) * package->header->entryCount;
	ByteArray* originalIndexes =
			huffmanDecode(L"Entry Indexes", data, encodedLen, decodedLen);
	deleteByteArray(encodedData);
	package->indexes = originalIndexes;
	return (package->indexes != NULL);
}

static void cleanupForEntry(wchar_t* name, wchar_t* path, ByteArray* encodedData, ByteArray* decodedData, bool success) {
	if (name) free(name);
	if (path) free(path);
	if (encodedData != NULL && encodedData != decodedData)
		deleteByteArray(encodedData);
	if (decodedData != NULL)
		deleteByteArray(decodedData);
	if (!success)
		encodingSwitchToNative();
}

static bool extractFiles(NexasPackage* package, const wchar_t* targetDir) {
	IndexEntry* indexes = (IndexEntry*)baData(package->indexes);
	u32 count = package->header->entryCount;

	/// The file names are stored in Shift-JIS.
	encodingSwitchToShiftJIS();

	for (u32 i = 0; i < count; ++i) {
		wchar_t* wName = toWCString(indexes[i].name);
		writeLog(LOG_VERBOSE, L"Entry %u: Name: %s, Offset: %u, ELen: %u, DLen: %u",
				i, wName, indexes[i].offset, indexes[i].encodedLen,
				indexes[i].decodedLen);
		if (fseek(package->file, indexes[i].offset, SEEK_SET) != 0) {
			writeLog(LOG_QUIET, L"ERROR: Entry %u: Name: %s, Unable to locate data!",
													i, wName);
			cleanupForEntry(wName, NULL, NULL, NULL, false);
			return false;
		}

		ByteArray* encodedData = newByteArray(indexes[i].encodedLen);
		ByteArray* decodedData = encodedData;
		if (fread(baData(encodedData), 1, indexes[i].encodedLen, package->file) != indexes[i].encodedLen) {
			writeLog(LOG_QUIET, L"ERROR: Entry %u: Name: %s, Unable to read data from package!",
							i, wName);
			cleanupForEntry(wName, NULL, encodedData, NULL, false);
			return false;
		}

		/**
		 * The stored data may actually be in the uncompressed form.
		 * (e.g. update.pac in Baldr Sky Dive 2 version 1.05)
		 * So we should do a test.
		 */
		unsigned long decodedLen = indexes[i].decodedLen;
		if (decodedLen > indexes[i].encodedLen) {
			decodedData = newByteArray(decodedLen);
			if (uncompress(baData(decodedData), &decodedLen, baData(encodedData), indexes[i].encodedLen) != Z_OK) {
				writeLog(LOG_QUIET, L"ERROR: Entry %u: Name: %s, Unable to extract data!",
					i, wName);
				cleanupForEntry(wName, NULL, encodedData, decodedData, false);
				return false;
			}
		}

		wchar_t* wPath = fsCombinePath(targetDir, wName);
		FILE* outFile = _wfopen(wPath, L"wb");
		if (outFile == NULL) {
			writeLog(LOG_QUIET,
					L"ERROR: Entry %u: Name: %s, Unable to open output file!",
						i, wName);
			cleanupForEntry(wName, wPath, encodedData, decodedData, false);
			return false;
		}
		if (fwrite(baData(decodedData), 1, indexes[i].decodedLen, outFile)
				!= indexes[i].decodedLen) {
			writeLog(LOG_QUIET,
					L"ERROR: Entry %u: Name: %s, Unable to write file content!",
						i, wName);
			cleanupForEntry(wName, wPath, encodedData, decodedData, false);
			fclose(outFile);
			return false;
		}
		fclose(outFile);
		writeLog(LOG_NORMAL, L"Unpacked: Entry %u: Name: %s", i, wName);
		cleanupForEntry(wName, wPath, encodedData, decodedData, true);
	}

	encodingSwitchToNative();
	return true;
}

bool unpackPackage(const wchar_t* packagePath, const wchar_t* targetDir) {
	wchar_t* aPackagePath = fsAbsolutePath(packagePath);
	wchar_t* aTargetDir = fsAbsolutePath(targetDir);
	writeLog(LOG_NORMAL, L"Now Unpacking package: %s", aPackagePath);
	writeLog(LOG_NORMAL, L"To Directory: %s", aTargetDir);
	if (!fsEnsureDirectoryExists(targetDir)) {
		writeLog(LOG_QUIET, L"ERROR: Target directory does not exist and could not be created.", targetDir);
		free(aPackagePath);
		free(aTargetDir);
		return false;
	}
	NexasPackage* package = openPackage(packagePath);
	if (!package) return false;
	bool result = validateHeader(package)
			&& decodeIndex(package)
			&& extractFiles(package, targetDir);
	closePackage(package);
	free(aPackagePath);
	free(aTargetDir);
	writeLog(LOG_NORMAL, L"%s",
			result ? L"Unpacking Finished." : L"Unpacking Failed.");
	return result;
}
