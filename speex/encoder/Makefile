
BIN = speexencoder
OBJS =  main.o \
        encoder.o \
        filename_utils.o

include ../../Makefile.platform

ifeq ($(BUILD_PLATFORM), windows32)
    SPEEX_DIR = ../../deps/win32/speex
else
    SPEEX_DIR = ../../deps/osx/x86_64/speex
endif

INCLUDES = -I$(SPEEX_DIR)/include
LDFLAGS = -L$(SPEEX_DIR)/lib

CC = gcc
COMMON_FLAGS = -O3 -Werror -Wall $(INCLUDES)
CCFLAGS = $(COMMON_FLAGS)
CFLAGS = $(COMMON_FLAGS)

ifeq ($(BUILD_PLATFORM), windows32)
    LDFLAGS += -static -lspeex -lstdc++
else
    LDFLAGS += -lspeex -lstdc++
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
