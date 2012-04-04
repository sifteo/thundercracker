APP = ninja_slide
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua

include $(TC_DIR)/sdk/Makefile.rules

ctags: 
	/usr/local/bin/ctags -Rf tags ../../include .

run: ctags all 
	../../../firmware/master/master-sim ninja_slide.elf

run_flash_stats: ctags all 
	../../../firmware/master/master-sim ninja_slide.elf --flash_stats
