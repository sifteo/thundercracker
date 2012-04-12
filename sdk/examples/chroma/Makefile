APP = chroma
TC_DIR = ../../..

include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o banner.o Bubbles.o cubewrapper.o FloatingScore.o Glimmer.o GridSlot.o game.o Intro.o Level.o Puzzle.o RockExplosion.o TimeKeeper.o
ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/sdk/Makefile.rules

GENERATED_FILES += PuzzleData.h

PUZZLE_TOOLS = \
	convertpuzzles.py

Puzzle.o: PuzzleData.h

PuzzleData.h: $(PUZZLE_TOOLS) puzzles.json
	python convertpuzzles.py puzzles.json PuzzleData.h
