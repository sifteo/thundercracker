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

hello: $(OBJS_lib) examples/hello.o examples/loadstream.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.bin
	objcopy -I binary $< -O elf64-x86-64 -B i386:x86-64 $@

.PHONY: clean

clean:
	rm -f $(BINS) src/*.o examples/*.o
