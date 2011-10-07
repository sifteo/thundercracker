
BIN = tc-siftulator

OBJS = \
	src/cube_cpu_core.o \
	src/cube_cpu_disasm.o \
	src/cube_cpu_irq.o \
	src/cube_cpu_opcodes.o \
	src/cube_debug_mainview.o \
	src/cube_debug_memeditor.o \
	src/cube_debug_popups.o \
	src/cube_debug.o \
	src/cube_flash_model.o \
	src/cube_hardware.o \
	src/cube_network.o \
	src/system.o \
	src/frontend.o \
	src/main.o \

CDEPS = src/*.h

CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += -lncurses $(shell sdl-config --libs)

OS := $(shell uname)
ifeq ($(OS),Darwin)
        LDFLAGS += -framework OpenGL
else
	LDFLAGS += -lGL
endif

CFLAGS += -O3 -g -Werror -Wall
LDFLAGS += -g -lstdc++
CCFLAGS := $(CFLAGS)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
