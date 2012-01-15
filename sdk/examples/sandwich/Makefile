ifndef TC_DIR
TC_DIR = ../../../
endif

APP = sandwichcraft
OBJS = \
	$(ASSETS).gen.o \
	gen_assets.o \
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
GENERATED_FILES += gen_assets.h
GENERATED_FILES += gen_assets.cpp


gen_assets.cpp: gen_assets

gen_assets.lua: gen_assets

gen_assets: sandwich.py tmx.py
	python -c "import sandwich; sandwich.export()"
	