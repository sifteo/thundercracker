SDK_DIR = ../..
MASTER_LIB = ../../../firmware/master.a

BIN = text
OBJS = main.o

CDEPS = $(SDK_DIR)/include/sifteo/*.h

CCFLAGS += -Os -g -Werror -I$(SDK_DIR)/include
LDFLAGS += -g

$(BIN): $(OBJS) $(MASTER_LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) *.o
