
TC_DIR := ..
MASTER_DIR := $(TC_DIR)/firmware/master

include $(TC_DIR)/Makefile.platform

BIN := swiss$(BIN_EXT)

OBJS := \
    main.o          \
    usbdevice.o     \
    fwloader.o      \
    profiler.o      \
    elfdebuginfo.o  \
    mappedfile.o    \
    installer.o

# include directories
INCLUDES := \
        -I. \
        -I /usr/include/libusb-1.0 \
        -I$(MASTER_DIR)/bootloader \
        -I$(MASTER_DIR)/common \
        -I$(TC_DIR)/sdk/include \
        -I$(TC_DIR)/tools/fwdeploy/src \
        -I$(DEPS_DIR)/libusbx/include

VERSION := $(shell git describe --tags)

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O2
# we currently define SIFTEO_SIMULATOR so we can use macros.h - revisit whether
# this deserves any reorg
CCFLAGS += -DSDK_VERSION=$(VERSION) -DSIFTEO_SIMULATOR -D__STDC_FORMAT_MACROS -DNOT_USERSPACE

# library paths
LDFLAGS := -L$(DEPS_DIR)/libusbx/lib -lusb-1.0
LDFLAGS += $(LIB_STDCPP)

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
