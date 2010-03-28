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
#include "LzssCode.h"
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
		writeLog(LOG_QUIET, L"ERROR: Cannot open the package file.");
		closePackage(package);
		return NULL;
	}
	writeLog(LOG_VERBOSE, L"Package Opened.");
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

	if (vtag != CONTENT_MAYBE_DEFLATE && vtag != CONTENT_LZSS) {
		writeLog(LOG_QUIET, L"ERROR: This PAC variant is not supported yet.");
		return false;
	}
	writeLog(LOG_NORMAL, L"Entry count: %u.", package->header->entryCount);
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

static bool readIndex(NexasPackage* package) {
	/// First, try to read plain text index (used in Baldr Force EXE, PAC variant 2).
	writeLog(LOG_VERBOSE, L"Trying to read the index as plain text.");
	u32 indexesLen = package->header->entryCount * sizeof(IndexEntry);
	fseek(package->file, 12, SEEK_SET);
	package->indexes = newByteArray(indexesLen);
	if (fread(baData(package->indexes), sizeof(byte), indexesLen, package->file) != indexesLen) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the 'plain text' index!");
		deleteByteArray(package->indexes);
		return false;
	}

	/**
	 * If the index data is valid, the packed file contents should be immediately following,
	 * or we can conclude that the real index data is at the end of the file and encoded.
	 */
	IndexEntry* indexes = (IndexEntry*)(baData(package->indexes));
	if (indexes[0].offset != 12 + indexesLen) {
		writeLog(LOG_VERBOSE, L"The index is invalid, trying to read encoded index.");
		return decodeIndex(package);
	}
	return true;
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
		writeLog(LOG_VERBOSE, L"Entry %u: %s, Offset: %u, ELen: %u, DLen: %u",
				i, wName, indexes[i].offset, indexes[i].encodedLen,
				indexes[i].decodedLen);
		if (fseek(package->file, indexes[i].offset, SEEK_SET) != 0) {
			writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to locate data!",
													i, wName);
			cleanupForEntry(wName, NULL, NULL, NULL, false);
			return false;
		}

		ByteArray* encodedData = newByteArray(indexes[i].encodedLen);
		ByteArray* decodedData = encodedData;
		if (fread(baData(encodedData), 1, indexes[i].encodedLen, package->file) != indexes[i].encodedLen) {
			writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to read data from package!",
							i, wName);
			cleanupForEntry(wName, NULL, encodedData, NULL, false);
			return false;
		}

		/**
		 * Now we support two PAC variants.
		 * The first is Variant 4, where contents are either uncompressed (like ogg files),
		 * or compressed using zlib (the deflate algorithm).
		 * We should do a test to determine if the particular file is compressed or not.
		 */
		if (package->header->variantTag == CONTENT_MAYBE_DEFLATE) {
			unsigned long decodedLen = indexes[i].decodedLen;
			if (decodedLen > indexes[i].encodedLen) {
				decodedData = newByteArray(decodedLen);
				if (uncompress(baData(decodedData), &decodedLen, baData(encodedData), indexes[i].encodedLen) != Z_OK) {
					writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to extract data!", i, wName);
					cleanupForEntry(wName, NULL, encodedData, decodedData, false);
					return false;
				}
			}
		} else if (package->header->variantTag == CONTENT_LZSS) {
			decodedData = lzssDecode(baData(encodedData), indexes[i].encodedLen, indexes[i].decodedLen);
			if (decodedData == NULL) {
				writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to extract data!", i, wName);
				cleanupForEntry(wName, NULL, encodedData, decodedData, false);
				return false;
			}
		}

		wchar_t* wPath = fsCombinePath(targetDir, wName);
		FILE* outFile = _wfopen(wPath, L"wb");
		if (outFile == NULL) {
			writeLog(LOG_QUIET,
					L"ERROR: Entry %u: %s, Unable to open output file!",
						i, wName);
			cleanupForEntry(wName, wPath, encodedData, decodedData, false);
			return false;
		}
		if (fwrite(baData(decodedData), 1, indexes[i].decodedLen, outFile)
				!= indexes[i].decodedLen) {
			writeLog(LOG_QUIET,
					L"ERROR: Entry %u: %s, Unable to write file content!",
						i, wName);
			cleanupForEntry(wName, wPath, encodedData, decodedData, false);
			fclose(outFile);
			return false;
		}
		fclose(outFile);
		writeLog(LOG_NORMAL, L"Unpacked: Entry %u: %s", i, wName);
		cleanupForEntry(wName, wPath, encodedData, decodedData, true);
	}

	encodingSwitchToNative();
	return true;
}

bool unpackPackage(const wchar_t* packagePath, const wchar_t* targetDir) {
	writeLog(LOG_NORMAL, L"Unpacking package: %s", packagePath);
	writeLog(LOG_NORMAL, L"To Directory: %s", targetDir);
	if (!fsEnsureDirectoryExists(targetDir)) {
		writeLog(LOG_QUIET, L"ERROR: Target directory does not exist and cannot be created.", targetDir);
		return false;
	}
	NexasPackage* package = openPackage(packagePath);
	if (!package) return false;
	bool result = validateHeader(package)
			&& readIndex(package)
			&& extractFiles(package, targetDir);
	closePackage(package);
	writeLog(LOG_NORMAL, (result) ? L"Unpacking Successful." : L"ERROR: Unpacking Failed.");
	return result;
}
