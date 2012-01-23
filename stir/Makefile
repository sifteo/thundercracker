BIN = stir

OBJS_lua = \
	src/lua/lapi.o src/lua/lcode.o src/lua/ldebug.o src/lua/ldo.o src/lua/ldump.o \
	src/lua/lfunc.o src/lua/lgc.o src/lua/llex.o src/lua/lmem.o src/lua/lobject.o \
	src/lua/lopcodes.o src/lua/lparser.o src/lua/lstate.o src/lua/lstring.o \
	src/lua/ltable.o src/lua/ltm.o src/lua/lundump.o src/lua/lvm.o src/lua/lzio.o \
	src/lua/lauxlib.o src/lua/lbaselib.o src/lua/ldblib.o src/lua/liolib.o \
	src/lua/lmathlib.o src/lua/loslib.o src/lua/ltablib.o src/lua/lstrlib.o \
	src/lua/loadlib.o src/lua/linit.o

OBJS = \
	src/script.o \
	src/proof.o \
	src/proof_html.o \
	src/asegwriter.o \
	src/cppwriter.o \
	src/imagestack.o \
	src/tile.o \
	src/tilecodec.o \
	src/color.o \
	src/command.o \
	src/logger.o \
	src/lodepng.o \
	src/sha1.o \
	src/speexencoder.o \
	$(OBJS_lua) \

TC_DIR := ..
include $(TC_DIR)/Makefile.platform

LDFLAGS += $(LIB_STDCPP)

# speex
SPEEX_DIR := $(DEPS_DIR)/speex
INCLUDES += -I$(SPEEX_DIR)/include
LDFLAGS += -L$(SPEEX_DIR)/lib
LDFLAGS += -lspeex

ifeq ($(BUILD_PLATFORM), windows32)
	OBJS += src/winres.o
else
	CFLAGS += -DLUA_USE_MKSTEMP
endif

DEPFILES := $(OBJS:.o=.d)
FIRMWARE_INC = $(TC_DIR)/firmware/include
CFLAGS += -O3 -g -ffast-math -Werror -Wall $(INCLUDES) -I$(FIRMWARE_INC) -MMD
LDFLAGS += -g -lm -lstdc++
CCFLAGS := $(CFLAGS)

all: $(BIN)

ifeq ($(BUILD_PLATFORM), windows32)
# We like having stripped binaries plus a separate file with
# debug info. Do that on Win32 too.
$(BIN): $(OBJS)
	$(CC) -o src/$(BIN)-d.exe $(OBJS) $(LDFLAGS)
	objcopy --only-keep-debug src/$(BIN)-d.exe $(BIN).dSYM
	strip -s src/$(BIN)-d.exe -o $(BIN).exe
	$(UPX) $(BIN).exe
else
# Default link
$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
endif	

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.rc
	$(WINDRES) -i $< -o $@

src/proof_html.cpp: src/proof_html.py
	$(PYTHON) $< $@

.PHONY: clean

clean:
	rm -f $(BIN) $(BIN).exe $(BIN).dSYM src/$(BIN)-d.exe $(OBJS) $(DEPFILES)

-include $(DEPFILES)
