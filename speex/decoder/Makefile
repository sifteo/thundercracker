
BIN = speexdecoder
OBJS = speexdecoder.o

CC = gcc
CCFLAGS = -Wall
LDFLAGS = -lspeex



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


