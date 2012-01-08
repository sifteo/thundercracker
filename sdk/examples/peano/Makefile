TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = peano.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o

BIN_STM32 = hello.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += PeanoTools.exe

PeanoTools.exe: PeanoTools/*.cs
	gmcs -debug PeanoTools/*.cs -out PeanoTools.exe -r:System.Drawing.dll

