SDK_DIR = ../..
MASTER_DIR = ../../../firmware/master
STIR = ../../../stir/stir
ASSETS = assets

BIN_SIM = hello.sim
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o
LIBS_SIM = $(MASTER_DIR)/master-sim.a

BIN_STM32 = hello.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o
LIBS_STM32 = $(MASTER_DIR)/master-stm32.a

BINS = $(BIN_SIM) $(BIN_STM32)
CDEPS = $(SDK_DIR)/include/sifteo/*.h $(STIR) $(ASSETS).lua *.png

include $(MASTER_DIR)/Makefile.rules

all: $(BINS)

$(BIN_SIM): $(OBJS_SIM) $(LIBS_SIM)
	$(CC_SIM) -o $@ $^ $(LDFLAGS_SIM)

$(BIN_STM32): $(OBJS_STM32) $(LIBS_STM32)
	$(CC_STM32) -o $@ $^ $(LDFLAGS_STM32)

$(ASSETS).gen.cpp: $(ASSETDEPS)
	$(STIR) $(ASSETS).lua -o $(ASSETS).gen.cpp -o $(ASSETS).gen.h -o $(ASSETS).html

.PHONY: clean all

clean:
	rm -f $(BINS) *.o $(ASSETS).gen.h $(ASSETS).gen.cpp $(ASSETS).html
