TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = buddies.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o CubeWrapper.sim.o App.sim.o Puzzle.sim.o

BIN_STM32 = buddies.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o CubeWrapper.stm32.o App.stm32.o Puzzle.stm32.o

ASSETDEPS += assets/*.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
