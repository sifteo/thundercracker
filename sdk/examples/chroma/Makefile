TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = chroma.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o main.sim.o banner.sim.o cubewrapper.sim.o GameOver.sim.o Glimmer.sim.o GridSlot.sim.o game.sim.o Intro.sim.o Level.sim.o Puzzle.sim.o TimeKeeper.sim.o

BIN_STM32 = chroma.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o main.stm32.o banner.stm32.o cubewrapper.stm32.o GameOver.stm32.o Glimmer.stm32.o GridSlot.stm32.o game.stm32.o Intro.stm32.o Level.stm32.o Puzzle.stm32.o TimeKeeper.stm32.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += PuzzleData.h

PUZZLE_TOOLS = \
	convertpuzzles.py

Puzzle.sim.o: PuzzleData.h
Puzzle.stm32.o: PuzzleData.h

PuzzleData.h: $(PUZZLE_TOOLS) puzzles.json
	python convertpuzzles.py puzzles.json PuzzleData.h	
