BIN = ../simcube

OBJS = \
	core.o \
	disasm.o \
	emu.o \
	mainview.o \
	memeditor.o \
	opcodes.o \
	options.o \
	popups.o \
	hardware.o \
	frontend.o \
	lcd.o \
	flash.o \
	spi.o \
	adc.o \
	irq.o \
	radio.o \
	network.o \

CDEPS = *.h

CFLAGS += -O3 -g -Werror
LDFLAGS += -g

CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += -lncurses $(shell sdl-config --libs)

OS := $(shell uname)
ifeq ($(OS),Darwin)
	CFLAGS += -framework OpenGL
	LDFLAGS += -framework OpenGL
else
	LDFLAGS += -lGL
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
