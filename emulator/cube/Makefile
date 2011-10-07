BIN = ../simcube

OBJS = \
	emu.o \
	core.o \
	disasm.o \
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
	i2c.o \
	accel.o \
	irq.o \
	radio.o \
	network.o \

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
	rm -f $(BIN)
