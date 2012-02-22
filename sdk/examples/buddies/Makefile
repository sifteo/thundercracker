TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

APP = buddies

OBJECTS = $(filter-out $(ASSETS).gen.cpp, $(wildcard *.cpp))

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o $(patsubst %.cpp, %.sim.o, $(OBJECTS))

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o $(patsubst %.cpp, %.stm32.o, $(OBJECTS))

ASSETDEPS += assets/*.png $(ASSETS).lua
CDEPS += *.h

include $(TC_DIR)/firmware/master/Makefile.rules
