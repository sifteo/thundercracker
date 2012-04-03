APP = ninja_slide
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua
CFLAGS += -working-directory "src"

include $(TC_DIR)/sdk/Makefile.rules

ctags: 
	/usr/local/bin/ctags -Rf tags ../../include .

run: ctags all 
	../../../firmware/master/master-sim ninja_slide.elf
