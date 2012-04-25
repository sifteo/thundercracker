APP = buddies


include $(SDK_DIR)/Makefile.defs

OBJECTS = $(filter-out $(ASSETS).gen.cpp, $(wildcard *.cpp))
OBJS = $(ASSETS).gen.o $(patsubst %.cpp, %.o, $(OBJECTS))

ASSETDEPS += assets/*.png $(ASSETS).lua
CDEPS += *.h

include $(SDK_DIR)/Makefile.rules

GENERATED_FILES += PuzzleData_GENERATED.h
PuzzleData.o: PuzzleData_GENERATED.h
PuzzleData_GENERATED.h: ConvertPuzzles.py PuzzleData.json
	python ConvertPuzzles.py PuzzleData.json PuzzleData_GENERATED.h