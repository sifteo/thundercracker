ifndef TC_DIR
TC_DIR = ../../../
endif

APP = sandwichcraft
OBJS = \
	$(ASSETS).gen.o \
	content.gen.o \
	main.o \
	Common.o \
	Dialog.o \
	DrawingHelpers.o \
	EdgeView.o \
	Game.o \
	Game_IntroCutscene.o \
	Game_WinScreen.o \
	Game_MainLoop.o \
	GameState.o \
	IdleView.o \
	InventoryView.o \
	Map.o \
	Map_NarrowPath.o \
	Map_BroadPath.o \
	MapHelpers.o \
	MinimapView.o \
	Player.o \
	Room.o \
	RoomView.o \
	Sokoblock.o \
	ViewSlot.o \

include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += **/*.png $(ASSETS).lua content.gen.lua
CDEPS += *.h 

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += content.gen.lua
GENERATED_FILES += content.gen.cpp
GENERATED_FILES += content_crc.txt

content.gen.cpp: content_crc.txt

content.gen.lua: content_crc.txt

content_crc.txt: *.py Content/*
	python sandwich.py
