
TC_DIR := ..
BIN_DIR := $(TC_DIR)/sdk/bin
MASTER_DIR := $(TC_DIR)/firmware/master

include $(TC_DIR)/Makefile.platform
include Makefile.defs

BIN := swiss

OBJS := \
    src/main.o          \
    src/progressbar.o   \
    src/tabularlist.o   \
    src/usbdevice.o     \
    src/fwloader.o      \
    src/profiler.o      \
    src/elfdebuginfo.o  \
    src/mappedfile.o    \
    src/installer.o     \
    src/manifest.o      \
    src/delete.o        \
    src/paircube.o      \
	src/reboot.o		\
	src/backup.o

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

include Makefile.rules
