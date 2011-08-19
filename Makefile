BIN = cube51-sim

OBJS = \
	src/core.o \
	src/disasm.o \
	src/emu.o \
	src/mainview.o \
	src/memeditor.o \
	src/opcodes.o \
	src/options.o \
	src/popups.o \
	src/hardware.o \
	src/lcd.o \
	src/flash.o \
	src/spi.o \

PY_PATH := /opt/local/bin:/usr/bin
PYTHON  := python2.7

CFLAGS += -O3 -g -Werror $(shell pkg-config --cflags sdl)
LDFLAGS += -g -lncurses $(shell pkg-config --libs sdl)

all: $(BIN) firmware flash.bin

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.PHONY: firmware clean

firmware:
	make -C firmware

flash.bin: assets/flashgen.py assets/*.png
	PATH=$(PY_PATH) $(PYTHON) $<

clean:
	rm -f $(BIN) $(OBJS) flash.bin *.ihx
	make -C firmware clean
