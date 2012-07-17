BIN = bootloader
TC_DIR = ../../..

LDSCRIPT := bootloader-target.ld

include $(TC_DIR)/firmware/master/Makefile.defs

# bootloader specific flags
CCFLAGS += -I. -DBOOTLOADER

# Sources specific to bootloader firmware
OBJS_STM32 = \
    $(MASTER_DIR)/stm32/setup.stm32.o               \
    $(MASTER_DIR)/stm32/vectors.stm32.o             \
    $(MASTER_DIR)/stm32/gpio.stm32.o                \
    $(MASTER_DIR)/stm32/powermanager.stm32.o        \
    $(MASTER_DIR)/stm32/systime.stm32.o             \
    $(MASTER_DIR)/bootloader/main.stm32.o           \
    $(MASTER_DIR)/bootloader/bootloader.stm32.o     \
    $(MASTER_DIR)/common/aes128.stm32.o             \
    $(MASTER_DIR)/stm32/stm32flash.stm32.o          \
    $(MASTER_DIR)/common/tasks.stm32.o              \
    $(MASTER_DIR)/stm32/usb/usbdevice.stm32.o       \
    $(MASTER_DIR)/stm32/usb/usbcore.stm32.o         \
    $(MASTER_DIR)/stm32/usb/usbcontrol.stm32.o      \
    $(MASTER_DIR)/stm32/usb/usbhardware_stm32otg.stm32.o

include $(MASTER_DIR)/Makefile.rules
