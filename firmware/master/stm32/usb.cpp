#include "usb.h"
#include <sifteo.h>
#include "hardware.h"
#include "usart.h"
#include "tasks.h"
#include "assetmanager.h"

#include "usb_init.h"
#include "usb_istr.h"
#include "otgd_fs_cal.h"
#include "otgd_fs_pcd.h"
#include "usb_sil.h"

static bool writeInProgress;
static uint32_t pendingOUTBytes;

/*
    Called from within Tasks::work() to process usb OUT data on the
    main thread.
    XXX: atomicity of pendingOUTBytes
*/
void Usb::handleOUTData(void *p) {
    (void)p;

    uint8_t buf[64];
    while (pendingOUTBytes > 0) {
        unsigned chunk = MIN(sizeof(buf), pendingOUTBytes);
        Usb::read(buf, chunk);
        AssetManager::onData(buf, chunk);
        pendingOUTBytes -= chunk;
    }
}

/*
    Called from within Tasks::work() to handle a usb IN event on the main thread.
    TBD whether this is actually helpful.
*/
void Usb::handleINData(void *p) {
    (void)p;
}

void Usb::init() {
    writeInProgress = false;
    pendingOUTBytes = 0;
    USB_Init();
}

int Usb::write(const uint8_t *buf, unsigned len)
{
    writeInProgress = true;
    USB_SIL_Write(Usb::Ep1In, (uint8_t*)buf, len);
    // this is quite terrible, but to avoid statically allocating more memory,
    // wait around till this has actually been written to ensure the data is still valid.
    while (writeInProgress);
    return 0;
}

int Usb::read(uint8_t *buf, unsigned len)
{
    USB_OTG_EP* ep = PCD_GetOutEP(3);
    if (len < ep->xfer_len) {
        // SIL API doesn't provide a way to specify the number of bytes to read.
        // must read the number that were last transferred - never more than
        // max packet size for this endpoint
        while (1);
    }
    len = ep->xfer_len;
    USB_SIL_Read(Usb::Ep3Out, buf);
    return len;
}

// defined in usb_istr.h, called in IRQ context
void EP1_IN_Callback(void)
{
    writeInProgress = false;
//    Tasks::setPending(Tasks::UsbIN, 0);
}

// defined in usb_istr.h, called in IRQ context
void EP3_OUT_Callback(void)
{
    // track the number of bytes waiting to be read, once we get around to it
    pendingOUTBytes += PCD_GetOutEP(3)->xfer_len;
    Tasks::setPending(Tasks::UsbOUT, 0);
}

IRQ_HANDLER ISR_UsbOtg_FS() {
    STM32_PCD_OTG_ISR_Handler();
}
