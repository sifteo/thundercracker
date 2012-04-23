APP = peano
SDK_DIR = ../..
include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o \
        puzzles.gen.o main.o Puzzle.o ShapeMask.o Token.o Fraction.o \
        TokenGroup.o Game.o \
        PuzzleDatabase.o SaveData.o TotalsCube.o \
        AudioPlayer.o View.o StingController.o \
        TokenView.o PuzzleController.o TiltFlowDetailView.o \
        InterstitialView.o MenuController.o TiltFlowMenu.o \
        TiltFlowView.o NarratorView.o DialogWindow.o \
        ConfirmationMenu.o InterstitialController.o PuzzleHelper.o \
        TiltFlowItem.o TutorialController.o VictoryController.o VictoryView.o \
        Skins.o

ASSETDEPS += *.png $(ASSETS).lua
ASSETS_BUILD_PROOF = TRUE

include $(SDK_DIR)/Makefile.rules

GENERATED_FILES += PeanoTools.exe

PeanoTools.exe: PeanoTools/*.cs
	gmcs -debug PeanoTools/*.cs -out PeanoTools.exe -r:System.Drawing.dll

GENERATED_FILES += puzzles.gen.cpp 
GENERATED_FILES += puzzles.gen.h
puzzles.gen.cpp puzzles.gen.h : puzzles.xml mungePuzzles.py
	python mungePuzzles.py
