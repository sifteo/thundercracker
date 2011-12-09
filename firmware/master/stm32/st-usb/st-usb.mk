

USB_DIR = $(TC_DIR)/firmware/master/stm32/st-usb

OBJS_USB_STM32 = \
    $(USB_DIR)/STM32_USB_Device_Library/Core/src/usbd_req.stm32.o \
    $(USB_DIR)/STM32_USB_Device_Library/Core/src/usbd_core.stm32.o \
    $(USB_DIR)/STM32_USB_Device_Library/Core/src/usbd_ioreq.stm32.o \
    $(USB_DIR)/STM32_USB_Device_Library/Class/cdc/src/usbd_cdc_core.stm32.o \
    $(USB_DIR)/STM32_USB_Device_Library/Class/cdc/src/usbd_cdc_core.stm32.o \
    $(USB_DIR)/STM32_USB_OTG_Driver/src/usb_dcd_int.stm32.o \
    $(USB_DIR)/STM32_USB_OTG_Driver/src/usb_core.stm32.o \
    $(USB_DIR)/STM32_USB_OTG_Driver/src/usb_dcd.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/misc.stm32.o

INC_USB_STM32 = \
    -I $(USB_DIR)/STM32_USB_Device_Library/Core/inc \
    -I $(USB_DIR)/STM32_USB_OTG_Driver/inc \
    -I $(USB_DIR)/STM32_USB_Device_Library/Class/cdc/inc \
    -I $(USB_DIR)/CMSIS/CM3/CoreSupport \
    -I $(USB_DIR)/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
    -I $(USB_DIR)/STM32F10x_StdPeriph_Driver/inc
