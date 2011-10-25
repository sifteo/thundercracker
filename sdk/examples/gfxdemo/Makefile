TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = gfxdemo.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o

BIN_STM32 = gfxdemo.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
