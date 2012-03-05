ifndef TC_DIR
TC_DIR = ../../../
endif

include $(TC_DIR)/firmware/master/Makefile.defs

APP = betterflow
OBJS = \
	$(ASSETS).gen.o \
	main.o \

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua

default: betterflow.sim
	tc-siftulator -n 3 &
	./betterflow.sim > output.txt

include $(TC_DIR)/firmware/master/Makefile.rules
