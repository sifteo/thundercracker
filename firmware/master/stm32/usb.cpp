#include "usb.h"
#include "hardware.h"
#include "usb_init.h"
#include "usb_istr.h"
#include "otgd_fs_cal.h"
#include "otgd_fs_pcd.h"
#include "usb_sil.h"

void Usb::init() {
    USB_Init();
}

// defined in usb_istr.h
void EP1_IN_Callback(void)
{

}

// defined in usb_istr.h
void EP3_OUT_Callback(void)
{
    uint8_t buffer[64];
    uint16_t count = USB_SIL_Read(EP3_OUT, buffer);

    USB_SIL_Write(EP1_IN, buffer, count);
}

IRQ_HANDLER ISR_UsbOtg_FS() {
    STM32_PCD_OTG_ISR_Handler();
}
