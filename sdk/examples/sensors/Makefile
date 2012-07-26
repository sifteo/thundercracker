APP = sensors

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += *.png $(ASSETS).lua
CCFLAGS += -DCUBE_ALLOCATION=24

include $(SDK_DIR)/Makefile.rules
