#include "usb.h"
#include "hardware.h"
#include "usb_init.h"
#include "usb_istr.h"

void Usb::init() {
    USB_Init();
}

IRQ_HANDLER ISR_UsbOtg_FS() {
    STM32_PCD_OTG_ISR_Handler();
}
