TC_DIR = ../../..
include $(TC_DIR)/firmware/master/Makefile.defs

BIN_SIM = channels.sim$(BIN_EXT)
OBJS_SIM = $(AUDIO).gen.sim.o main.sim.o

BIN_STM32 = nyan.stm32
OBJS_STM32 = $(AUDIO).gen.stm32.o main.stm32.o

AUDIODEPS += *.raw $(AUDIO).txt

include $(TC_DIR)/firmware/master/Makefile.rules
