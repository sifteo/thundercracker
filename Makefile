BIN = cube51-sim

OBJS = \
	src/core.o \
	src/disasm.o \
	src/emu.o \
	src/logicboard.o \
	src/mainview.o \
	src/memeditor.o \
	src/opcodes.o \
	src/options.o \
	src/popups.o \
	src/hardware.o \
	src/lcd.o \
	src/flash.o \

CFLAGS += -O3 -Werror $(shell pkg-config --cflags sdl)
LDFLAGS += -lncurses $(shell pkg-config --libs sdl)

all: $(BIN) firmware flash.bin

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.PHONY: firmware clean

firmware:
	make -C firmware

flash.bin: assets/flashgen.py assets/*.png
	python $<

clean:
	rm -f $(BIN) $(OBJS) flash.bin
	make -C firmware clean
