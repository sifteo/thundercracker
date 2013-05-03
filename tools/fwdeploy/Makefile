
TC_DIR := ../..
MASTER_DIR := $(TC_DIR)/firmware/master

include $(TC_DIR)/Makefile.platform

BIN := fwdeploy$(BIN_EXT)

OBJS := \
    src/main.o                          \
    src/deployer.o                      \
    src/encrypter.o                     \
    src/inspect.o                       \
    src/crc.o                           \
    src/securerandom.o                  \
    $(MASTER_DIR)/common/aes128.o

# include directories
INCLUDES := \
        -Isrc \
        -I$(MASTER_DIR)/bootloader \
        -I$(MASTER_DIR)/common

VERSION := $(shell git describe --tags)

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O2
# we currently define SIFTEO_SIMULATOR so we can use macros.h - revisit whether
# this deserves any reorg
CCFLAGS += -DSDK_VERSION=$(VERSION) -DSIFTEO_SIMULATOR -D__STDC_FORMAT_MACROS

# library paths
LDFLAGS := $(LIB_STDCPP)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(INCLUDES) $(CCFLAGS) $(FLAGS) $*.cpp -o $*.o

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o

.PHONY: clean

clean:
	rm -Rf $(BIN)
	rm -Rf $(OBJS) $(DEPFILES) $(BIN).dSYM
