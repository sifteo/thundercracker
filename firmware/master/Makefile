BIN = master-stm32
TC_DIR = ../..

include Makefile.objs
include Makefile.defs

# Sources for normal master firmware
OBJS_STM32 += \
    $(MASTER_DIR)/stm32/main.stm32.o                    \
    $(MASTER_DIR)/stm32/realtimeclock.stm32.o           \
    $(MASTER_DIR)/stm32/sampleprofiler.stm32.o          \
    $(MASTER_DIR)/stm32/adc.stm32.o

FLAGS += -I$(MASTER_DIR)/bootloader

include Makefile.rules
