ifndef TC_DIR
TC_DIR = ../../../
endif

APP = sandwichcraft
OBJS = \
	$(ASSETS).gen.o \
	gen_mapdata.o \
	main.o \
	Base.o \
	Dialog.o \
	Game.o \
	GameView.o \
	Map.o \
	Player.o \

include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua gen_assets.lua
CDEPS += *.h 

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += MapTool.exe
GENERATED_FILES += gen_assets.lua
GENERATED_FILES += gen_mapdata.h
GENERATED_FILES += gen_mapdata.cpp


gen_mapdata.cpp: MapTool.exe $(ASSETDEPS) *.tmx
	mono MapTool.exe -gen_cxx

gen_assets.lua: MapTool.exe *.tmx
	mono MapTool.exe -gen_lua

MapTool.exe: Tools/*.cs
	gmcs -debug Tools/*.cs -out:MapTool.exe -r:System.Drawing.dll

