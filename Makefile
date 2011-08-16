BIN = cube51-sim

SRC = core.c  disasm.c  emu.c  logicboard.c  mainview.c \
      memeditor.c  opcodes.c  options.c  popups.c

all: $(BIN)

$(BIN): $(SRC) *.h
	gcc -o $(BIN) $(SRC) -lncurses -g
