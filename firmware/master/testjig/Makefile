BIN = testjig
TC_DIR = ../../..

BOARD := BOARD_TEST_JIG

include $(TC_DIR)/firmware/master/Makefile.defs

# testjig specific flags
FLAGS += -I. -I../bootloader

# Sources specific to testjig firmware
OBJS_STM32 := \
    $(MASTER_DIR)/stm32/setup.stm32.o               \
    $(MASTER_DIR)/stm32/vectors.stm32.o             \
    $(MASTER_DIR)/stm32/gpio.stm32.o                \
    $(MASTER_DIR)/stm32/systime.stm32.o             \
    $(MASTER_DIR)/stm32/usart.stm32.o               \
    $(MASTER_DIR)/common/tasks.stm32.o              \
    $(MASTER_DIR)/testjig/main.stm32.o              \
    $(MASTER_DIR)/testjig/testjig.stm32.o           \
    $(MASTER_DIR)/testjig/pulse_rx.stm32.o          \
    $(MASTER_DIR)/stm32/neighbor_tx.stm32.o         \
    $(MASTER_DIR)/stm32/neighbor_rx.stm32.o         \
    $(MASTER_DIR)/stm32/dac.stm32.o                 \
    $(MASTER_DIR)/stm32/dma.stm32.o                 \
    $(MASTER_DIR)/stm32/hwtimer.stm32.o             \
    $(MASTER_DIR)/stm32/i2c.stm32.o                 \
    $(MASTER_DIR)/stm32/adc.stm32.o                 \
    $(MASTER_DIR)/stm32/sysinfo.stm32.o             \
    $(MASTER_DIR)/stm32/usb/usbdevice.stm32.o       \
    $(MASTER_DIR)/stm32/usb/usbcore.stm32.o         \
    $(MASTER_DIR)/stm32/usb/usbcontrol.stm32.o      \
    $(MASTER_DIR)/stm32/usb/usbhardware_stm32otg.stm32.o

# Standard library parts used by the testjig
OBJS_STM32 += \
    $(MASTER_DIR)/stm32/newlib/libc/machine/arm/memcpy.stm32.o  \
    $(MASTER_DIR)/stm32/newlib/libc/machine/arm/memset.stm32.o  \
    $(MASTER_DIR)/stm32/newlib/libc/machine/arm/strcpy.stm32.o  \
    $(MASTER_DIR)/stm32/newlib/libc/machine/arm/strcmp.stm32.o  \
    $(MASTER_DIR)/stm32/newlib/libc/machine/arm/strlen.stm32.o  \
    $(MASTER_DIR)/stm32/newlib/libc/string/memchr.stm32.o       \
    $(MASTER_DIR)/stm32/newlib/libc/string/memcmp.stm32.o       \
    $(MASTER_DIR)/stm32/newlib/libc/string/memmove.stm32.o

include $(MASTER_DIR)/Makefile.rules
