TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = text.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o  main.sim.o

BIN_STM32 = text.stm32
OBJS_STM32 = main.stm32.o

CDEPS += *.h

ASSETS_BUILD_PROOF = TRUE

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
