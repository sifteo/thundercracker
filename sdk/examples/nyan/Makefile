TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = nyan.sim$(BIN_EXT)
#OBJS_SIM = $(ASSETS).gen.sim.o $(AUDIO).gen.sim.o main.sim.o
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o

BIN_STM32 = nyan.stm32
#OBJS_STM32 = $(ASSETS).gen.stm32.o $(AUDIO).gen.stm32.o main.stm32.o
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o

ASSETDEPS += *.png $(ASSETS).lua
AUDIODEPS += *.raw $(AUDIO).txt

include $(TC_DIR)/firmware/master/Makefile.rules
