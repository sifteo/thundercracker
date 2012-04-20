APP = ninja_slide
SDK_DIR = ../..
include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua sounds/*.raw

include $(SDK_DIR)/Makefile.rules

ctags: 
	/usr/local/bin/ctags -Rf tags $(SDK_DIR)/include .

run_prog: ctags run

