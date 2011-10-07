BIN = ../simcube

OBJS = \
	cpu_core.o \
	cpu_disasm.o \
	cpu_irq.o \
	cpu_opcodes.o \
	cube_hardware.o \
	flash_model.o \
	debug_mainview.o \
	debug_memeditor.o \
	debug_popups.o \
	network.o \
	main.o \
	frontend.o \

CDEPS = *.h

CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += -lncurses $(shell sdl-config --libs)

OS := $(shell uname)
ifeq ($(OS),Darwin)
        LDFLAGS += -framework OpenGL
else
	LDFLAGS += -lGL
endif

CFLAGS += -O3 -g -Werror -Wall
LDFLAGS += -g
CCFLAGS := $(CFLAGS)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
