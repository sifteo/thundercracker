APP = ninja_slide
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua sounds/*.raw

include $(TC_DIR)/sdk/Makefile.rules

ctags: 
	/usr/local/bin/ctags -Rf tags ../../include .
echo:
	echo $(ASSETDEPS)

run: ctags all 
	osascript -e 'tell application "tc-siftulator"' -e 'activate' -e 'end tell'
	../../../firmware/master/master-sim ninja_slide.elf

siftulator:
	$(TC_DIR)/emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator &

run_flash_stats: ctags all 
	osascript -e 'tell application "tc-siftulator"' -e 'activate' -e 'end tell'
	../../../firmware/master/master-sim ninja_slide.elf --flash_stats
