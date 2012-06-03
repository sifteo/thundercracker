BIN = bootloader.elf
TC_DIR = ../../..

LDSCRIPT := bootloader-target.ld

include $(TC_DIR)/firmware/master/Makefile.objs
include $(TC_DIR)/firmware/master/Makefile.defs

# bootloader specific flags
CCFLAGS += -I.

# Sources specific to bootloader firmware
OBJS_STM32 += \
    $(MASTER_DIR)/bootloader/main.stm32.o           \
    $(MASTER_DIR)/bootloader/bootloader.stm32.o     \
    $(MASTER_DIR)/common/aes128.stm32.o             \
    $(MASTER_DIR)/stm32/stm32flash.stm32.o

include $(MASTER_DIR)/Makefile.rules
