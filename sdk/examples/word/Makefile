APP = word
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o StateMachine.o \
GameStateMachine.o WordGame.o \
CubeStateMachine.o Dictionary.o \
PrototypeWordList.o SavedData.o \
Utility.o Anim.o \
qsort.o bsearch.o

ASSETDEPS += *.png $(ASSETS).lua

include $(TC_DIR)/sdk/Makefile.rules

run: all 
	/usr/local/bin/ctags -Rf tags $(TC_DIR)/sdk/include .
	$(TC_DIR)/emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator /Users/winstonw/sifteo/thundercracker/sdk/examples/word/word.elf

