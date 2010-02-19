EXE_TARGET = zbspac.exe

TXTS = License.txt Readme.txt
PROJECT_FILE = Makefile .project .cproject
SRC_DIST = zbspac-src.7z
BIN_DIST = zbspac-bin.7z

CC = gcc
LD = ld
CFLAGS = -O3 -std=c99 -Werror -Wall -pedantic -pedantic-errors
LIBS = -lz
DIST_MAKE = 7za a

SRCS = $(wildcard *.c)
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
	del $(OBJS) $(EXE_TARGET) $(SRC_DIST) $(BIN_DIST)
	