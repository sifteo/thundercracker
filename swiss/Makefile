
TC_DIR := ..

include $(TC_DIR)/Makefile.platform

BIN := swiss$(BIN_EXT)

OBJS := \
    main.o          \
    usbdevice.o     \
    profiler.o

# include directories
INCLUDES = -I. -I$(DEPS_DIR)/libusbx/include

VERSION := $(shell git describe --tags)
ifeq ($(VERSION), )
VERSION := "\"\""
endif

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O2
CCFLAGS += -DSDK_VERSION=$(VERSION)

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
