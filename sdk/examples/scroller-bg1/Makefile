TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = scroller-bg1.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o

BIN_STM32 = scroller-bg1.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
