ifndef SDK_DIR
SDK_DIR = ../..
endif

APP = sandwichcraft
include $(SDK_DIR)/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	content.gen.o \
	main.o \
	Bomb.o \
	Common.o \
	Dialog.o \
	DrawingHelpers.o \
	EdgeView.o \
	Game.o \
	Game_CheckMapNeighbors.o \
	Game_Events.o \
	Game_IntroCutscene.o \
	Game_MainLoop.o \
	Game_MainMenu.o \
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
	Viewport.o \

ASSETDEPS += *.png $(ASSETS).lua content.gen.lua

GENERATED_FILES += content.gen.lua
GENERATED_FILES += content.gen.cpp
GENERATED_FILES += content_crc.txt

include $(SDK_DIR)/Makefile.rules

content.gen.cpp: content_crc.txt

content.gen.lua: content_crc.txt

content_crc.txt: *.py Content/*
	python sandwich.py
