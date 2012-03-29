APP = membrane
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	main.o \
	game.o \
	cube.o \
	flavor.o \
	particle.o \
	portal.o \
	loading.o

ASSETDEPS += *.png $(ASSETS).lua
CDEPS += *.h

include $(TC_DIR)/sdk/Makefile.rules
