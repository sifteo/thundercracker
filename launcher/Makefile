APP = launcher

include $(SDK_DIR)/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	src/main.o \
	src/mainmenu.o \
	src/elfmainmenuitem.o \
	src/assetloaderbypassdelegate.o \
	src/cuberange.o

ASSETDEPS += images/*.png sounds/*.xm $(ASSETS).lua
CDEPS += src/*.h

include $(SDK_DIR)/Makefile.rules
