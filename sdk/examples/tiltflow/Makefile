TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = tiltflow.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o cubewrapper.sim.o game.sim.o bg1helper.sim.o

BIN_STM32 = tiltflow.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o cubewrapper.stm.o game.stm.o bg1helper.stm.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
