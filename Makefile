BIN = cube51-sim

OBJS = \
	core.o \
	disasm.o \
	emu.o \
	logicboard.o \
	mainview.o \
	memeditor.o \
	opcodes.o \
	options.o \
	popups.o

CFLAGS += -O3 -Werror
LDFLAGS += -lncurses

all: $(BIN)

clean:
	rm -f $(BIN) $(OBJS)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)