TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = word.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o StateMachine.sim.o \
GameStateMachine.sim.o State.sim.o ScoredGameState.sim.o WordGame.sim.o \
CubeStateMachine.sim.o CubeState.sim.o  ScoredCubeState.sim.o

BIN_STM32 = word.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o StateMachine.stm32.o \
GameStateMachine.stm32.o State.stm32.o ScoredGameState.stm32.o \
WordGame.stm32.o CubeStateMachine.stm32.o ScoredCubeState.stm32.o \
CubeState.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
