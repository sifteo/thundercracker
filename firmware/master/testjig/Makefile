BIN = testjig
TC_DIR = ../../..

include $(TC_DIR)/firmware/master/Makefile.objs
include $(TC_DIR)/firmware/master/Makefile.defs

# testjig specific flags
CCFLAGS += -I. -I../bootloader -DBOARD=BOARD_TEST_JIG

# Sources specific to testjig firmware
OBJS_STM32 += \
    $(MASTER_DIR)/testjig/main.stm32.o                  \
    $(MASTER_DIR)/testjig/testjig.stm32.o               \
    $(MASTER_DIR)/stm32/i2c.stm32.o                     \
    $(MASTER_DIR)/stm32/adc.stm32.o                     \
    $(MASTER_DIR)/stm32/sampleprofiler.stm32.o

include $(MASTER_DIR)/Makefile.rules
