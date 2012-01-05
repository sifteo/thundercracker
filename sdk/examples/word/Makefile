TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = word.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o $(AUDIO).gen.sim.o main.sim.o StateMachine.sim.o \
GameStateMachine.sim.o State.sim.o ScoredGameState.sim.o WordGame.sim.o \
CubeStateMachine.sim.o CubeState.sim.o  ScoredCubeState.sim.o Dictionary.sim.o \
ScoredCubeState_NotWord.sim.o ScoredCubeState_NewWord.sim.o \
ScoredCubeState_OldWord.sim.o PrototypeWordList.sim.o \
ScoredGameState_EndOfRound.sim.o ScoredCubeState_EndOfRound.sim.o \
SavedData.sim.o TitleGameState.sim.o TitleCubeState.sim.o \
TitleExitCubeState.sim.o TileTransparencyLookup.sim.o \
TileTransparencyLookupData.sim.o PartialAnimationData.sim.o \
ScoredCubeState_Shuffle.sim.o ScoredGameState_Shuffle.sim.o \
ScoredCubeState_StartOfRound.sim.o ScoredGameState_StartOfRound.sim.o

BIN_STM32 = word.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o $(AUDIO).gen.stm32.o main.stm32.o StateMachine.stm32.o \
GameStateMachine.stm32.o State.stm32.o ScoredGameState.stm32.o \
WordGame.stm32.o CubeStateMachine.stm32.o ScoredCubeState.stm32.o \
CubeState.stm32.o Dictionary.stm32.o ScoredCubeState_NewWord.stm32.o \
ScoredCubeState_NotWord.stm32.o ScoredCubeState_OldWord.stm32.o \
PrototypeWordList.stm32.o ScoredGameState_EndOfRound.stm32.o \
ScoredCubeState_EndOfRound.stm32.o SavedData.stm32.o TitleGameState.stm32.o \
TitleCubeState.stm32.o TitleExitCubeState.stm32.o TileTransparencyLookup.stm32.o \
TileTransparencyLookupData.stm32.o PartialAnimationData.stm32.o \
ScoredCubeState_Shuffle.stm32.o ScoredGameState_Shuffle.stm32.o \
ScoredCubeState_StartOfRound.stm32.o ScoredGameState_StartOfRound.stm32.o

ASSETDEPS += *.png $(ASSETS).lua
AUDIODEPS += *.raw $(AUDIO).txt

include $(TC_DIR)/firmware/master/Makefile.rules
