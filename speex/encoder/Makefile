
BIN = speexencoder
OBJS = main.o \
        encoder.o \
        filename_utils.o

ifeq ($(OS),Windows_NT)
    SPEEX_INSTALL = ../prebuilt/win32
endif

INCLUDES = -I$(SPEEX_INSTALL)/include

CC = gcc
CCFLAGS = -Wall $(INCLUDES)
CFLAGS = -Wall $(INCLUDES)
LDFLAGS = -L$(SPEEX_INSTALL)/lib -static -lspeex -lstdc++

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
