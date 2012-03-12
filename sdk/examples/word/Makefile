APP = word
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o StateMachine.o \
GameStateMachine.o State.o ScoredGameState.o WordGame.o \
CubeStateMachine.o CubeState.o  ScoredCubeState.o Dictionary.o \
ScoredCubeState_NotWord.o ScoredCubeState_NewWord.o \
ScoredCubeState_OldWord.o PrototypeWordList.o \
ScoredGameState_EndOfRound.o ScoredCubeState_EndOfRound.o \
SavedData.o TitleGameState.o TitleCubeState.o \
TitleExitCubeState.o TileTransparencyLookup.o \
TileTransparencyLookupData.o PartialAnimationData.o \
ScoredCubeState_Shuffle.o ScoredGameState_Shuffle.o \
ScoredCubeState_StartOfRound.o ScoredGameState_StartOfRound.o \
Utility.o Anim.o StoryGameState_CityProgression.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/sdk/Makefile.rules
