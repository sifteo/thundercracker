APP = ninja_slide
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua sounds/*.raw

include $(TC_DIR)/sdk/Makefile.rules

siftulator:
	$(TC_DIR)/emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator &

run: all 
	osascript -e 'tell application "tc-siftulator"' -e 'activate' -e 'end tell'
	$(TC_DIR)/firmware/master/master-sim ninja_slide.elf

ctags: 
	/usr/local/bin/ctags -Rf tags $(TC_DIR)/sdk/include .

run_prog: ctags run

