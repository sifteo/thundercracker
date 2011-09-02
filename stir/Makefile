BINS = stir lsdec

OBJS_stir = \
	src/color.o \
	src/command.o \
	src/lodepng.o \
	src/tile.o \
	src/tilecodec.o \
	src/logger.o \

OBJS_lsdec = \
	src/lsdec.o \
	src/lodepng.o \

CDEPS = src/*.h

CCFLAGS += -O3 -g -Werror -Isrc
LDFLAGS += -g -lm -lstdc++

all: $(BINS)

stir: $(OBJS_stir)
	$(CC) -o $@ $^ $(LDFLAGS)

lsdec: $(OBJS_lsdec)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BINS) $(OBJS_stir) $(OBJS_lsdec)
