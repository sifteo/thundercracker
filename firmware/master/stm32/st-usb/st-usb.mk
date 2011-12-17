

USB_DIR = $(TC_DIR)/firmware/master/stm32/st-usb

OBJS_USB_STM32 = \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/otgd_fs_cal.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/otgd_fs_dev.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/otgd_fs_int.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/otgd_fs_pcd.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_core.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_init.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_int.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_mem.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_regs.stm32.o \
    $(USB_DIR)/STM32_USB-FS-Device_Driver/src/usb_sil.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.stm32.o \
    $(USB_DIR)/STM32F10x_StdPeriph_Driver/src/misc.stm32.o \
    $(USB_DIR)/VirtualComPort/hw_config.stm32.o \
    $(USB_DIR)/VirtualComPort/usb_desc.stm32.o \
    $(USB_DIR)/VirtualComPort/usb_endp.stm32.o \
    $(USB_DIR)/VirtualComPort/usb_istr.stm32.o \
    $(USB_DIR)/VirtualComPort/usb_prop.stm32.o \
    $(USB_DIR)/VirtualComPort/usb_pwr.stm32.o

INC_USB_STM32 = \
    -I $(USB_DIR)/VirtualComPort \
    -I $(USB_DIR)/STM32_USB-FS-Device_Driver/inc \
    -I $(USB_DIR)/CMSIS/CM3/CoreSupport \
    -I $(USB_DIR)/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
    -I $(USB_DIR)/STM32F10x_StdPeriph_Driver/inc
