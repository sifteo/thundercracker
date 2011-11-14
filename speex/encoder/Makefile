
BIN = speexencoder
OBJS =  main.o \
        encoder.o \
        filename_utils.o

ifeq ($(OS), Windows_NT)
    SPEEX_INSTALL = ../../deps/win32/speex
    INCLUDES = -I$(SPEEX_INSTALL)/include
    LDFLAGS = -L$(SPEEX_INSTALL)/lib -static
endif

CC = gcc
COMMON_FLAGS = -O3 -Werror -Wall $(INCLUDES)
CCFLAGS = $(COMMON_FLAGS)
CFLAGS = $(COMMON_FLAGS)
LDFLAGS += -lspeex -lstdc++

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
