APP = launcher

include $(SDK_DIR)/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	src/shared.o \
	src/main.o \
	src/mainmenu.o \
	src/elfmainmenuitem.o \
	src/assetloaderbypassdelegate.o \
	src/defaultloadinganimation.o \
	src/cuberange.o \
	src/nineblock.o \
	src/applet/getgames.o \
	src/applet/status.o

ASSETDEPS += images/*.png sounds/*.xm $(ASSETS).lua
CDEPS += src/*.h
CCFLAGS += -Isrc

# support standalone launcher build
ifeq ($(SDK_VERSION), )
CCFLAGS += -DSDK_VERSION=$(shell git describe --tags)
endif

include $(SDK_DIR)/Makefile.rules
