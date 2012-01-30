ifndef TC_DIR
TC_DIR = ../../../
endif

APP = sandwichcraft
OBJS = \
	$(ASSETS).gen.o \
	content.gen.o \
	main.o \
	Base.o \
	Dialog.o \
	Game.o \
	GameState.o \
	GameView.o \
	Map.o \
	Map_NarrowPath.o \
	Map_BroadPath.o \
	Player.o \
	Room.o

include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua content.gen.lua
ifdef MAX_HAX
ASSETDEPS -= content.gen.lua
endif

CDEPS += *.h 

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += content.gen.lua
GENERATED_FILES += content.gen.cpp
GENERATED_FILES += content_crc.txt

content.gen.cpp: content_crc.txt

content.gen.lua: content_crc.txt

CONTENT_TOOLS = \
	sandwich.py \
	sandwich_dialog.py \
	sandwich_map.py \
	sandwich_script.py \
	tmx.py \
	misc.py


content_crc.txt: $(CONTENT_TOOLS) *.tmx game-script.xml dialog-database.xml
	python sandwich.py
