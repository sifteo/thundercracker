
TC_DIR := ../..
DEPS_DIR := $(TC_DIR)/deps/osx

LIBOUT := usbdevice.a

OBJS := \
    usbdevice.o

# include directories
INCLUDES = -I. -I$(DEPS_DIR)/libusbx/include

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O2

# library paths
LIBS = -L$(DEPS_DIR)/libusbx/lib

all: $(LIBOUT)

$(LIBOUT): $(OBJS)
	ar rcs $(LIBOUT) $(OBJS)

%.o: %.cpp
	$(CC) -c $(INCLUDES) $(CCFLAGS) $*.cpp -o $*.o

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o

.PHONY: clean

clean:
	rm -Rf $(LIBOUT)
	rm -Rf $(OBJS)
