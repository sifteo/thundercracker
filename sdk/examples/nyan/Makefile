TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = nyan.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o audio.gen.sim.o

BIN_STM32 = hello.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o audio.gen.stm32.o

ASSETDEPS += *.png $(ASSETS).lua
AUDIODEPS += *.raw $(AUDIO).txt

include $(TC_DIR)/firmware/master/Makefile.rules
