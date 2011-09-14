BIN = master.a
SDK_DIR = ../../sdk

OBJS = \
	common/syscall.o \
	common/runtime.o \
	common/radio.o \
	common/cube.o \
	sim/radio_nethub.o \
	sim/main.o \

CDEPS = common/*.h $(SDK_DIR)/include/sifteo/*.h

CCFLAGS += -Os -g -Werror -I$(SDK_DIR)/include -Icommon

$(BIN): $(OBJS)
	rm -f $@
	$(AR) r $@ $(OBJS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)
