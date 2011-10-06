BIN = ../simcube

SRC = \
	emu.c \
	core.c \
	disasm.c \
	mainview.c \
	memeditor.c \
	opcodes.c \
	options.c \
	popups.c \
	hardware.c \
	frontend.c \
	lcd.c \
	flash.c \
	spi.c \
	adc.c \
	i2c.c \
	accel.c \
	irq.c \
	radio.c \
	network.c \

CDEPS = *.h

CFLAGS += -O3 -g -Werror
LDFLAGS += -g

CFLAGS += $(shell sdl-config --cflags)

OS := $(shell uname)

ifneq ( $(findstr MINGW32_NT, $(OS)) , "" )  
      OS := windows32
endif

ifneq ($(OS),windows32)
	LDFLAGS += -lncurses $(shell sdl-config --libs)
endif

ifeq ($(OS),Darwin)
        LDFLAGS += -framework OpenGL

        # Performance is MUCH better if we can optimize between modules!
        # Otherwise, we spend a lot of time on glue between emulator modules,
        # which could have been optimized out. This hits especially hard for
        # the tick() entry points on our individual subsystem emulations.
        #
        # Unfortunately, there's an annoying GCC bug that's breaking this
        # on most recent non-Apple builds of GCC, so I'm only enabling this
        # on Mac OS at the moment:
        #   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=33763
        #
	CFLAGS += -combine -fwhole-program

else ifeq ($(OS),windows32)
	LDFLAGS += ../../deps/win32/pdcurses/pdcurses.lib -lGL -lws2_32 $(shell sdl-config --libs)
else
	LDFLAGS += -lGL
endif

all: $(BIN)

$(BIN): $(SRC) $(CDEPS)

	#on Windows I seem to need to move all this to the end.
	#perhaps that would work on all platforms?
ifeq ($(OS),windows32)
	$(CC) -o $@ $(CFLAGS) $(SRC) $(LDFLAGS) 
else
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $(SRC)
endif



.PHONY: clean

clean:
	rm -f $(BIN).*
	rm -rf *.o