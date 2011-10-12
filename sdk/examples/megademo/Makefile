TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = megademo.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o cubewrapper.sim.o GridSlot.sim.o game.sim.o

BIN_STM32 = megademo.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o cubewrapper.stm32.o GridSlot.stm32.o game.stm32.o

CDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
