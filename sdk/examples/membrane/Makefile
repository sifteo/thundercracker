APP = membrane

include $(SDK_DIR)/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	main.o \
	game.o \
	cube.o \
	flavor.o \
	particle.o \
	portal.o

ASSETDEPS += *.png $(ASSETS).lua
CDEPS += *.h

include $(SDK_DIR)/Makefile.rules
