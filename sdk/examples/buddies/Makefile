TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

APP = buddies

BIN_SIM = $(APP).sim$(BIN_EXT)

# TODO: Can't we just glob all .cpp files?
OBJS_SIM = \
    $(ASSETS).gen.sim.o \
    App.sim.o \
    CubeWrapper.sim.o \
    Puzzle.sim.o \
    PuzzleData.sim.o \
    main.sim.o

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS_SIM,.sim.o=.stm32.o)

ASSETDEPS += assets/*.png $(ASSETS).lua
CDEPS += *.h

include $(TC_DIR)/firmware/master/Makefile.rules
