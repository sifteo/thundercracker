ifndef TC_DIR
TC_DIR = ../../..
endif

APP = sandwichcraft
include $(TC_DIR)/sdk/Makefile.defs

OBJS = \
	$(ASSETS).gen.o \
	content.gen.o \
	main.o \
	Common.o \
	Dialog.o \
	DrawingHelpers.o \
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
	MinimapView.o \
	Player.o \
	Room.o \
	RoomView.o \
	ViewSlot.o \

ASSETDEPS += *.png $(ASSETS).lua content.gen.lua
ifdef MAX_HAX
ASSETDEPS -= content.gen.lua
endif

CDEPS += *.h 

include $(TC_DIR)/sdk/Makefile.rules

GENERATED_FILES += content.gen.lua
GENERATED_FILES += content.gen.cpp
GENERATED_FILES += content_crc.txt

content.gen.cpp: content_crc.txt

content.gen.lua: content_crc.txt

CONTENT_TOOLS = \
	sandwich.py \
	sandwich_dialog.py \
	sandwich_item.py \
	sandwich_map.py \
	sandwich_quest.py \
	tmx.py \
	misc.py


content_crc.txt: $(CONTENT_TOOLS) Content/*
	python sandwich.py
