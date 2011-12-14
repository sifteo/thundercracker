TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = tiltflow.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o cubewrapper.sim.o menucontroller.sim.o tiltFlowMenu.sim.o

BIN_STM32 = tiltflow.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o cubewrapper.stm32.o menucontroller.stm32.o tiltFlowMenu.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
