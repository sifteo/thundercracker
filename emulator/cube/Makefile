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
	irq.c \
	radio.c \
	network.c \

CDEPS = *.h

CFLAGS += -O3 -g -Werror
LDFLAGS += -g

# Performance is MUCH better if we can optimize between modules!
# Otherwise, we spend a lot of time on glue between emulator modules,
# which could have been optimized out. This hits especially hard for
# the tick() entry points on our individual subsystem emulations.
CFLAGS += -combine -fwhole-program

CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += -lncurses $(shell sdl-config --libs)

OS := $(shell uname)
ifeq ($(OS),Darwin)
	LDFLAGS += -framework OpenGL
else
	LDFLAGS += -lGL
endif

all: $(BIN)

$(BIN): $(SRC) $(CDEPS)
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $(SRC)

.PHONY: clean

clean:
	rm -f $(BIN)
