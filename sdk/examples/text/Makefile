SDK_DIR = ../..
MASTER_DIR = ../../../firmware/master

BIN_SIM = text.sim
OBJS_SIM = main.sim.o
LIBS_SIM = $(MASTER_DIR)/master-sim.a

BIN_STM32 = text.stm32
OBJS_STM32 = main.stm32.o
LIBS_STM32 = $(MASTER_DIR)/master-stm32.a

BINS = $(BIN_SIM) $(BIN_STM32)
CDEPS = $(SDK_DIR)/include/sifteo/*.h *.h

include $(MASTER_DIR)/Makefile.rules

all: $(BINS)

$(BIN_SIM): $(OBJS_SIM) $(LIBS_SIM)
	$(CC_SIM) -o $@ $^ $(LDFLAGS_SIM)

$(BIN_STM32): $(OBJS_STM32) $(LIBS_STM32)
	$(CC_STM32) -o $@ $^ $(LDFLAGS_STM32)

.PHONY: clean all

clean:
	rm -f $(BINS) *.o
