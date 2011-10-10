BINS = stir

OBJS_lua = \
	src/lua/lapi.o src/lua/lcode.o src/lua/ldebug.o src/lua/ldo.o src/lua/ldump.o \
	src/lua/lfunc.o src/lua/lgc.o src/lua/llex.o src/lua/lmem.o src/lua/lobject.o \
	src/lua/lopcodes.o src/lua/lparser.o src/lua/lstate.o src/lua/lstring.o \
	src/lua/ltable.o src/lua/ltm.o src/lua/lundump.o src/lua/lvm.o src/lua/lzio.o \
	src/lua/lauxlib.o src/lua/lbaselib.o src/lua/ldblib.o src/lua/liolib.o \
	src/lua/lmathlib.o src/lua/loslib.o src/lua/ltablib.o src/lua/lstrlib.o \
	src/lua/loadlib.o src/lua/linit.o

OBJS_stir = \
	src/script.o \
	src/proof.o \
	src/proof_html.o \
	src/cppwriter.o \
	src/imagestack.o \
	src/tile.o \
	src/tilecodec.o \
	src/color.o \
	src/command.o \
	src/logger.o \
	src/lodepng.o \
	src/sha1.o \
	$(OBJS_lua) \

FIRMWARE_INC = ../firmware/include
CDEPS = src/*.h $(FIRMWARE_INC)/*.h

PYTHON := python
UNAME  := $(shell uname)

ifeq ($(findstring MINGW32_NT,$(UNAME)),MINGW32_NT)  
      UNAME := windows32
endif

CFLAGS += -O3 -g -ffast-math -Werror -Wall -I$(FIRMWARE_INC)
ifneq ($(UNAME), windows32)
    CFLAGS += -DLUA_USE_MKSTEMP
endif
LDFLAGS += -g -lm -lstdc++
CCFLAGS := $(CFLAGS)

all: $(BINS)

stir: $(OBJS_stir)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

src/proof_html.cpp: src/proof_html.py
	$(PYTHON) $< $@

.PHONY: clean

clean:
	rm -f $(BINS) $(OBJS_stir) $(OBJS_lsdec)
