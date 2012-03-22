APP = buddies
TC_DIR = ../../..
include $(TC_DIR)/sdk/Makefile.defs

OBJECTS = $(filter-out $(ASSETS).gen.cpp, $(wildcard *.cpp))
OBJS = $(ASSETS).gen.o $(patsubst %.cpp, %.o, $(OBJECTS))
ASSETDEPS += assets/*.png $(ASSETS).lua

include $(TC_DIR)/sdk/Makefile.rules
