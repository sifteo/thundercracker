TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = word.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o StateMachine.sim.o \
GameStateMachine.sim.o State.sim.o ScoredGameState.sim.o WordGame.sim.o \
CubeStateMachine.sim.o CubeState.sim.o  ScoredCubeState.sim.o Dictionary.sim.o \
ScoredCubeState_NotWord.sim.o ScoredCubeState_NewWord.sim.o \
ScoredCubeState_OldWord.sim.o PrototypeWordList.sim.o \
ScoredGameState_EndOfRound.sim.o ScoredCubeState_EndOfRound.sim.o

BIN_STM32 = word.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o StateMachine.stm32.o \
GameStateMachine.stm32.o State.stm32.o ScoredGameState.stm32.o \
WordGame.stm32.o CubeStateMachine.stm32.o ScoredCubeState.stm32.o \
CubeState.stm32.o Dictionary.stm32.o ScoredCubeState_NewWord.stm32.o \
ScoredCubeState_NotWord.stm32.o ScoredCubeState_OldWord.stm32.o \
PrototypeWordList.stm32.o ScoredGameState_EndOfRound.stm32.o \
ScoredCubeState_EndOfRound.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules
