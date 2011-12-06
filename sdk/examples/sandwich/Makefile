APP = sandwichcraft
OBJS = \
	$(ASSETS).gen.o \
	gen_mapdata.o \
	main.o \
	Game.o \
	GameView.o \
	Base.o \
	Player.o \
	Map.o \
	Enemy.o \

include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua gen_assets.lua *.tmx
CDEPS += *.h 

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += MapTool.exe
GENERATED_FILES += gen_assets.lua
GENERATED_FILES += gen_mapdata.h
GENERATED_FILES += gen_mapdata.cpp


gen_mapdata.cpp: MapTool.exe $(ASSETDEPS)
	mono MapTool.exe -gen_cxx

gen_assets.lua: MapTool.exe
	mono MapTool.exe -gen_lua

MapTool.exe: Tools/MapTool/*.cs
	gmcs -debug Tools/MapTool/*.cs -out:MapTool.exe -r:System.Drawing.dll

