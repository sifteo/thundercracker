TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = selectormenu.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o MenuController.sim.o TFcubewrapper.sim.o TiltFlowMenu.sim.o

BIN_STM32 = selectormenu.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o MenuController.stm32.o TFcubewrapper.stm32.o TiltFlowMenu.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
