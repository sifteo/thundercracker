APP = membrane
OBJS = \
	$(ASSETS).gen.o \
	main.o \
	game.o \
	cube.o \
	flavor.o \
	particle.o \
	portal.o \
	loading.o \

TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua
CDEPS += *.h

include $(TC_DIR)/firmware/master/Makefile.rules
