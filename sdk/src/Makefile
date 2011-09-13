BIN = tcsdk.a
SDK_DIR = ..

OBJS = \
	radio.o \
	radio_nethub.o \
	cube.o \
	system.o \

CDEPS = *.h $(SDK_DIR)/include/sifteo/*.h

CCFLAGS += -Os -g -Werror -I$(SDK_DIR)/include

$(BIN): $(OBJS)
	rm -f $@
	$(AR) r $@ $(OBJS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
