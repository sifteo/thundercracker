TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = nyan.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o audio.gen.sim.o

BIN_STM32 = hello.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o audio.gen.stm32.o

ASSETDEPS += audio.txt *.raw *.png $(ASSETS).lua audio.gen.cpp

include $(TC_DIR)/firmware/master/Makefile.rules

#audio.gen.cpp: audio.txt *.raw
#	speexencoder -o audio.gen.cpp -o audio.gen.h audio.txt
