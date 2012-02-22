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
    App.stm32.o \
    CubeWrapper.stm32.o \
    Puzzle.stm32.o \
    PuzzleData.stm32.o \
    main.stm32.o

ASSETDEPS += assets/*.png $(ASSETS).lua
CDEPS += *.h

include $(TC_DIR)/firmware/master/Makefile.rules
