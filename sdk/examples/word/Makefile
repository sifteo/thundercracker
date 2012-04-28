APP = word

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o StateMachine.o \
GameStateMachine.o WordGame.o \
CubeStateMachine.o Dictionary.o \
PrototypeWordList.o SavedData.o \
Utility.o Anim.o Dialog.o \
qsort.o bsearch.o

ASSETDEPS += *.png $(ASSETS).lua

include $(SDK_DIR)/Makefile.rules

run: all 
	/usr/local/bin/ctags -Rf tags $(SDK_DIR)/include .
	$(SDK_DIR)/bin/siftulator.app/Contents/MacOS/siftulator /Users/winstonw/sifteo/thundercracker/sdk/examples/word/word.elf

