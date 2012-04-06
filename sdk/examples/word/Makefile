TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = word.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o StateMachine.sim.o \
GameStateMachine.sim.o WordGame.sim.o \
CubeStateMachine.sim.o Dictionary.sim.o \
PrototypeWordList.sim.o \
SavedData.sim.o \
TileTransparencyLookup.sim.o \
TileTransparencyLookupData.sim.o PartialAnimationData.sim.o \
Utility.sim.o Anim.sim.o

BIN_STM32 = word.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o StateMachine.stm32.o \
GameStateMachine.stm32.o \
WordGame.stm32.o CubeStateMachine.stm32.o \
Dictionary.stm32.o \
PrototypeWordList.stm32.o \
SavedData.stm32.o \
TileTransparencyLookup.stm32.o \
TileTransparencyLookupData.stm32.o PartialAnimationData.stm32.o \
Utility.stm32.o Anim.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
