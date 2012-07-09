BIN = rftest
TC_DIR = ../../..

include $(TC_DIR)/firmware/master/Makefile.objs
include $(TC_DIR)/firmware/master/Makefile.defs

# Sources for rftest firmware
OBJS_STM32 += \
    $(MASTER_DIR)/rftest/main.stm32.o   \
    $(MASTER_DIR)/stm32/sampleprofiler.stm32.o

# rftest specific flags
CCFLAGS += -I. -I../bootloader -DRFTEST

include $(TC_DIR)/firmware/master/Makefile.rules
