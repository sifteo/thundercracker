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
	Player.o \

include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = $(APP).sim$(BIN_EXT)
OBJS_SIM = $(OBJS:.o=.sim.o)

BIN_STM32 = $(APP).stm32
OBJS_STM32 = $(OBJS:.o=.stm32.o)

ASSETDEPS += *.png $(ASSETS).lua content.gen.lua
CDEPS += *.h 

include $(TC_DIR)/firmware/master/Makefile.rules

GENERATED_FILES += content.gen.lua
GENERATED_FILES += content.gen.cpp


content.gen.cpp: content

content.gen.lua: content

CONTENT_TOOLS = \
	sandwich.py \
	sandwich_dialog.py \
	sandwich_map.py \
	sandwich_script.py \
	tmx.py


content: *.tmx game-script.xml dialog-database.xml
	python -c "import sandwich; sandwich.export()"
	