TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = chroma.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o cubewrapper.sim.o

BIN_STM32 = chroma.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o cubewrapper.stm32.o

CDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
