/**
 * @file		NexasPacker.c
 * @brief		Implementation of the packer.
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <wchar.h>
#include <io.h>
#include <direct.h>

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

static inline bool shouldZip(wchar_t* filename) {
	static const wchar_t* targetExts[] = { L".ogg" };
	for (i8 i = 0; i < 1; ++i) {
		wchar_t* cmpptr = filename + wcslen(filename) - wcslen(targetExts[i]);
		if (wcscmp(cmpptr, targetExts[i]) == 0) {
			return false;
		}
	}
	return true;
}

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
	if (!(package->file = _wfopen(packagePath, L"wb"))) {
		writeLog(LOG_QUIET, L"ERROR: Cannot open the package file.");
		closePackage(package);
		return NULL;
	}
	writeLog(LOG_VERBOSE, L"Package Opened.");
	return package;
}

static bool determineEntryCountAndWriteHeader(NexasPackage* package, const wchar_t* sourceDir, bool isBfeFormat) {
	writeLog(LOG_VERBOSE, L"Generating package header......");
	package->header = malloc(sizeof(Header));
	memcpy(package->header->typeTag, "PAC", 3);
	package->header->magicByte = 0;
	package->header->variantTag = isBfeFormat ? CONTENT_LZSS : CONTENT_MAYBE_DEFLATE;
	package->header->entryCount = 0;

	writeLog(LOG_VERBOSE, L"Moving into source directory......");
	if (_wchdir(sourceDir) != 0) {
		writeLog(LOG_QUIET, L"ERROR: Unable to read the source directory!");
		return false;
	}

	struct _wfinddata_t foundFile;
	intptr_t handle = _wfindfirst(L"*", &foundFile);
	int status = 0;
	while (status == 0) {
		if ((foundFile.attrib & _A_SUBDIR) == 0)
			++(package->header->entryCount);
		status = _wfindnext(handle, &foundFile);
	}
	_findclose(handle);

	writeLog(LOG_NORMAL, L"Found %u entries in the source directory.",
			package->header->entryCount);

	if (package->header->entryCount == 0) {
		writeLog(LOG_QUIET, L"ERROR: There is nothing to pack!");
		return false;
	}

	/// Write the header.
	if (fwrite(package->header, sizeof(Header), 1, package->file) != 1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write to the target package!");
		return false;
	}

	writeLog(LOG_VERBOSE, L"Package Header Written.");

	return true;
}

static bool recordAndWriteEntries(NexasPackage* package, bool isBfeFormat) {
	package->indexes = newByteArray(package->header->entryCount * sizeof(IndexEntry));
	IndexEntry* indexes = (IndexEntry*)baData(package->indexes);

	u32 i = 0;
	u32 offset = 12;

	if (isBfeFormat) {
		/// This PAC Variant puts index first, but now we do not know
		/// the index, so we reserve the space.
		u32 len = baLength(package->indexes);
		for (u32 i = 0; i < len; ++i) {
			if (fputc('\0', package->file) == EOF) {
				writeLog(LOG_QUIET, L"ERROR: Unable to reserve space for the index!");
				return false;
			}
		}
		offset += len;
	}

	struct _wfinddata_t foundFile;
	intptr_t handle = _wfindfirst(L"*", &foundFile);
	int status = 0;
	while (status == 0) {
		if ((foundFile.attrib & _A_SUBDIR) == 0) {
			char* fname = toMBString(foundFile.name, L"japanese");
			if (strlen(fname) >= 64) {
				writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, The file name is too long!", i, foundFile.name);
				free(fname);
				return false;
			}
			strncpy(indexes[i].name, fname, 64);
			free(fname);

			indexes[i].encodedLen = foundFile.size;
			indexes[i].decodedLen = foundFile.size;
			indexes[i].offset = offset;
			writeLog(LOG_VERBOSE, L"Entry %u: %s, Offset: %u, OLen: %u",
					i, foundFile.name, indexes[i].offset, indexes[i].decodedLen);

			FILE* infile = _wfopen(foundFile.name, L"rb");
			byte* decodedData = malloc(indexes[i].decodedLen);

			if (fread(decodedData, 1, indexes[i].decodedLen, infile) != indexes[i].decodedLen) {
				writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to read the file!", i, foundFile.name);
				free(decodedData);
				fclose(infile);
				return false;
			}
			fclose(infile);

			byte* encodedData = NULL;
			ByteArray* encodedArray = NULL;

			if (isBfeFormat) {
				encodedArray = lzssEncode(decodedData, indexes[i].decodedLen);
				encodedData = baData(encodedArray);
				indexes[i].encodedLen = baLength(encodedArray);
			} else if (shouldZip(foundFile.name)) {
				encodedData = malloc(indexes[i].decodedLen);
				unsigned long len = indexes[i].encodedLen;
				if (compress(encodedData, &len, decodedData, indexes[i].decodedLen) != Z_OK) {
					free(encodedData);
					free(decodedData);
					return false;
				}
				indexes[i].encodedLen = len;
				writeLog(LOG_VERBOSE, L"Entry %u is compressed: ELen: %u", i, len);
			} else {
				encodedData = decodedData;
			}
			offset += indexes[i].encodedLen;
			writeLog(LOG_VERBOSE, L"Entry %u: ELen: %u", i, indexes[i].encodedLen);

			if (fwrite(encodedData, 1, indexes[i].encodedLen, package->file) != indexes[i].encodedLen) {
				writeLog(LOG_QUIET, L"ERROR: Entry %u: %s, Unable to write to the package!", i, foundFile.name);
				if (encodedArray != NULL) {
					deleteByteArray(encodedArray);
				} else if (encodedData != decodedData) {
					free(encodedData);
				}
				free(decodedData);
				return false;
			}

			if (encodedArray != NULL) {
				deleteByteArray(encodedArray);
			} else if (encodedData != decodedData) {
				free(encodedData);
			}
			free(decodedData);

			writeLog(LOG_NORMAL, L"Packed: Entry %u: %s.", i, foundFile.name);
			++i;
		}
		status = _wfindnext(handle, &foundFile);
	}
	_findclose(handle);
	return true;
}

static bool writeBfeIndex(NexasPackage* package) {
	writeLog(LOG_VERBOSE, L"Writing plain text index.");
	fseek(package->file, 12, SEEK_SET);
	if (fwrite(baData(package->indexes), 1, baLength(package->indexes), package->file) != baLength(package->indexes)) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write the indexes to the package!");
		return false;
	}
	writeLog(LOG_VERBOSE, L"Written plain text index, length is: %u.", baLength(package->indexes));
	return true;
}

static bool writeIndexes(NexasPackage* package, bool isBfeFormat) {
	if (isBfeFormat)
		return writeBfeIndex(package);
	ByteArray* encodedIndexes =
			huffmanEncode(L"Entry Indexes", baData(package->indexes), baLength(package->indexes));
	if (encodedIndexes == NULL) {
		writeLog(LOG_QUIET, L"ERROR: Unable to encode the indexes!");
		return false;
	}

	byte* encodedData = baData(encodedIndexes);
	u32 encodedLen = baLength(encodedIndexes);
	writeLog(LOG_VERBOSE, L"The length of the compressed index is %u.", encodedLen);
	/// Important: XOR encryption!
	for (u32 i = 0; i < encodedLen; ++i) {
		encodedData[i] ^= 0xFF;
	}

	if (fwrite(encodedData, 1, encodedLen, package->file) != baLength(encodedIndexes)) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write the indexes to the package!");
		return false;
	}

	if (fwrite(&encodedLen, 4, 1, package->file) != 1) {
		writeLog(LOG_QUIET, L"ERROR: Unable to write the index length to the package!");
		return false;
	}
	return true;
}

bool packPackage(const wchar_t* sourceDir, const wchar_t* packagePath, bool isBfeFormat) {
	writeLog(LOG_NORMAL, L"Packing files under directory: %s", sourceDir);
	writeLog(LOG_NORMAL, L"To package: %s", packagePath);
	NexasPackage* package = openPackage(packagePath);
	if (!package) return false;
	bool result = determineEntryCountAndWriteHeader(package, sourceDir, isBfeFormat)
			&& recordAndWriteEntries(package, isBfeFormat)
			&& writeIndexes(package, isBfeFormat);
	closePackage(package);
	writeLog(LOG_NORMAL, (result) ? L"Packing Successful." : L"ERROR: Packing Failed.");
	return result;
}
