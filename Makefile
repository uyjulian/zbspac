EXE_TARGET = zbspac.exe

TXTS = License.txt Readme.txt Instructions.txt PackageFormat.txt ScriptTxtFormat.txt
PROJECT_FILE = Makefile .project .cproject
SRC_DIST = zbspac-src.7z
BIN_DIST = zbspac-bin.7z

CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld
CFLAGS = -O2 -std=c99 -Werror -Wall -pedantic -pedantic-errors -Iexternal/zlib
LIBS = -static -static-libstdc++ -static-libgcc
DIST_MAKE = 7za a

ZLIB_SRCS = external/zlib/adler32.c external/zlib/compress.c external/zlib/crc32.c external/zlib/deflate.c external/zlib/gzclose.c external/zlib/gzlib.c external/zlib/gzread.c external/zlib/gzwrite.c external/zlib/infback.c external/zlib/inffast.c external/zlib/inflate.c external/zlib/inftrees.c external/zlib/trees.c external/zlib/uncompr.c external/zlib/zutil.c
SRCS = $(wildcard *.c) $(ZLIB_SRCS)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c, %.o, $(SRCS)) 

all: $(EXE_TARGET)

$(EXE_TARGET): $(OBJS)
	$(CC) -o $(EXE_TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
src_dist: $(SRC_DIST)

bin_dist: $(BIN_DIST)

$(SRC_DIST): $(SRCS) $(HEADERS) $(TXTS) $(PROJECT_FILE)
	$(DIST_MAKE) $(SRC_DIST) $(SRCS) $(HEADERS) $(TXTS) $(PROJECT_FILE)

$(BIN_DIST): $(EXE_TARGET) $(TXTS)
	$(DIST_MAKE) $(BIN_DIST) $(EXE_TARGET) $(TXTS)

.PHONY: clean
	
clean:
	$(RM) $(OBJS) $(EXE_TARGET) $(SRC_DIST) $(BIN_DIST)
	