TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = chroma.sim$(BIN_EXT)
OBJS_SIM = $(ASSETS).gen.sim.o $(AUDIO).gen.sim.o main.sim.o banner.sim.o cubewrapper.sim.o GameOver.sim.o Glimmer.sim.o GridSlot.sim.o game.sim.o Intro.sim.o sprite.sim.o TimeKeeper.sim.o
#menu controller stuff
OBJS_SIM += MenuController.sim.o TFcubewrapper.sim.o TiltFlowMenu.sim.o

BIN_STM32 = chroma.stm32
OBJS_STM32 = $(ASSETS).gen.stm32.o $(AUDIO).gen.stm32.o main.stm32.o banner.stm32.o cubewrapper.stm32.o GameOver.stm32.o Glimmer.stm32.o GridSlot.stm32.o game.stm32.o Intro.stm32.o sprite.stm32.o TimeKeeper.stm32.o
#menu controller stuff
OBJS_STM32 += MenuController.stm32.o TFcubewrapper.stm32.o TiltFlowMenu.stm32.o

ASSETDEPS += *.png $(ASSETS).lua
AUDIODEPS += *.raw $(AUDIO).txt

include $(TC_DIR)/firmware/master/Makefile.rules
