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

CFLAGS += -O3 -Werror
LDFLAGS += -lncurses

all: $(BIN) firmware

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.PHONY: firmware clean

firmware:
	make -C firmware

clean:
	rm -f $(BIN) $(OBJS)
	make -C firmware clean
