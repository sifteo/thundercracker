TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = peano.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o \
	puzzles.gen.sim.o main.sim.o Puzzle.sim.o ShapeMask.sim.o Token.sim.o Fraction.sim.o \
        TokenGroup.sim.o Game.sim.o \
        PuzzleDatabase.sim.o SaveData.sim.o TotalsCube.sim.o \
        AudioPlayer.sim.o View.sim.o StingController.sim.o \
	TokenView.sim.o PuzzleController.sim.o TiltFlowDetailView.sim.o \
	InterstitialView.sim.o MenuController.sim.o TiltFlowMenu.sim.o \
        TiltFlowView.sim.o NarratorView.sim.o DialogWindow.sim.o \
        ConfirmationMenu.sim.o InterstitialController.sim.o PuzzleHelper.sim.o \
        TiltFlowItem.sim.o TutorialController.sim.o VictoryController.sim.o VictoryView.sim.o \
        Skins.sim.o

BIN_STM32 = peano.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o \
        puzzles.gen.stm32.o main.stm32.o ShapeMask.stm32.o Puzzle.stm32.o Token.stm32.o Fraction.stm32.o \
        TokenGroup.stm32.o Game.stm32.o \
        PuzzleDatabase.stm32.o SaveData.stm32.o TotalsCube.stm32.o \
        AudioPlayer.stm32.o View.stm32.o StingController.stm32.o \
	TokenView.stm32.o PuzzleController.stm32.o TiltFlowDetailView.stm32.o \
	InterstitialView.stm32.o MenuController.stm32.o TiltFlowMenu.stm32.o \
        TiltFlowView.stm32.o NarratorView.stm32.o DialogWindow.stm32.o \
        ConfirmationMenu.stm32.o InterstitialController.stm32.o PuzzleHelper.stm32.o \
        TiltFlowItem.stm32.o TutorialController.stm32.o VictoryController.stm32.o VictoryView.stm32.o \
        Skins.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

ASSETS_BUILD_PROOF = TRUE


include $(TC_DIR)/firmware/master/Makefile.rules


GENERATED_FILES += PeanoTools.exe

PeanoTools.exe: PeanoTools/*.cs
	gmcs -debug PeanoTools/*.cs -out PeanoTools.exe -r:System.Drawing.dll


GENERATED_FILES += puzzles.gen.cpp 
GENERATED_FILES += puzzles.gen.h
puzzles.gen.cpp puzzles.gen.h : puzzles.xml mungePuzzles.py dummy.txt
	python mungePuzzles.py

GENERATED_FILES += dummy.txt

dummy.txt : assets.gen.h assets.gen.cpp
	sed '/Major[NEWS]\;/s/const\ //g' -i assets.gen.h
	sed '/Major[NEWS]_tiles/s/const\ //g' -i assets.gen.cpp
	echo "dummy" > dummy.txt
