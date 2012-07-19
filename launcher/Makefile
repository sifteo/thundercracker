APP = launcher

TC_DIR := ..
include $(TC_DIR)/Makefile.platform
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
	src/applet/getgames.o

ASSETDEPS += images/*.png sounds/*.xm $(ASSETS).lua
CDEPS += src/*.h
CCFLAGS += -Isrc

# support standalone launcher build for quicker development iteration.
# disable this on windows, since when the launcher is getting built from
# the top level Makefile, all SDK exes get built with a restricted
# PATH, and Git is not typically on there. Edit as you please during development.
ifneq ($(BUILD_PLATFORM), windows32)
	ifeq ($(SDK_VERSION), )
		SDK_VERSION := $(shell git describe --tags)
		CCFLAGS += -DSDK_VERSION=$(SDK_VERSION)
	endif
endif

include $(SDK_DIR)/Makefile.rules
