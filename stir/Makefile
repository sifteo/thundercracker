TC_DIR := ..
include $(TC_DIR)/Makefile.platform

BIN_DIR := $(TC_DIR)/sdk/bin
BIN := stir
all: $(BIN_DIR)/$(BIN)$(BIN_EXT)

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
	src/cppwriter.o \
	src/imagestack.o \
	src/tile.o \
	src/tilecodec.o \
	src/color.o \
	src/command.o \
	src/logger.o \
	src/lodepng.o \
	src/sha1.o \
	src/audioencoder.o \
	src/dubencoder.o \
	src/tracker.o \
	$(OBJS_lua) \

LDFLAGS += $(LIB_STDCPP)

ifeq ($(BUILD_PLATFORM), windows32)
	OBJS += src/winres.o
else
	CFLAGS += -DLUA_USE_MKSTEMP
endif

DEPFILES := $(OBJS:.o=.d)
FIRMWARE_INC = $(TC_DIR)/firmware/include
SYS_INC = $(TC_DIR)/sdk/include
CFLAGS += -DNOT_USERSPACE

# XXX: We'd like to use -O4 (link-time optimization) but bibble
#      seems to be unable to link its own .o files when LTO is enabled...
FLAGS += -O3 -g

CFLAGS += $(FLAGS) -ffast-math -Werror -Wall $(INCLUDES) -I$(FIRMWARE_INC) -I$(SYS_INC) -MMD
LDFLAGS += $(FLAGS) -lm -lstdc++
CCFLAGS := $(CFLAGS)

# Symbol table trimming
ifeq ($(BUILD_PLATFORM), Darwin)
	LDFLAGS += -Xlinker -unexported_symbol -Xlinker "*"
endif

# Disable implicit rules. Speeds up make, especially on Windows.
.SUFFIXES:

ifeq ($(BUILD_PLATFORM), windows32)
# Win32 Link: Stripped and packed binaries, and separate debug symbols.
# upx doesn't like overwriting the output file - rm it in case we're not building after clean
$(BIN_DIR)/$(BIN)$(BIN_EXT): $(OBJS)
	$(CC) -o "src/$(BIN)-d.exe" $(OBJS) $(LDFLAGS)
	objcopy --only-keep-debug "src/$(BIN)-d.exe" "$(BIN).dSYM"
	strip -s "src/$(BIN)-d.exe" -o "src/$(BIN)-stripped.exe"
	rm -f "$(BIN_DIR)/$(BIN).exe"
	$(UPX) -o "$(BIN_DIR)/$(BIN).exe" "src/$(BIN)-stripped.exe"
endif

ifeq ($(BUILD_PLATFORM), Linux)
$(BIN_DIR)/$(BIN): $(OBJS)
	$(CC) -o "src/$(BIN)-d" $(OBJS) $(LDFLAGS)
	strip -o "src/$(BIN)-stripped" "src/$(BIN)-d"
	rm -f "$(BIN_DIR)/$(BIN)"
	$(UPX) -o "$(BIN_DIR)/$(BIN)" "src/$(BIN)-stripped"
endif

ifeq ($(BUILD_PLATFORM), Darwin)
$(BIN_DIR)/$(BIN): $(OBJS)
	$(CC) -o src/$(BIN)-d $(OBJS) $(LDFLAGS)
	strip -o $(BIN_DIR)/$(BIN) src/$(BIN)-d
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
	rm -f $(BIN_DIR)/$(BIN)$(BIN_EXT) $(BIN).dSYM src/$(BIN)-d.exe $(OBJS) $(DEPFILES)

-include $(DEPFILES)
