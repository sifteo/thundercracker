BINS = hello

OBJS_lib = \
	src/radio.o \
	src/radio_nethub.o \
	src/cube.o \
	src/system.o \

CDEPS = src/*.h include/sifteo/*.h

CCFLAGS += -g -Werror -Iinclude
LDFLAGS += -g

all: $(BINS)

hello: examples/hello.o $(OBJS_lib)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BINS) src/*.o examples/*.o
