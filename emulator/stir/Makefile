BIN = stir

OBJS = \
	src/color.o \
	src/command.o \
	src/lodepng.o \
	src/tile.o \

CDEPS = src/*.h

CCFLAGS += -O3 -g -Werror -Isrc
LDFLAGS += -g -lm -lstdc++

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
