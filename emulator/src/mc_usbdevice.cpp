#include "mc_usbdevice.h"
#include "mc_usbhardware.h"
#include "usbprotocol.h"

void UsbDevice::handleOUTData()
{
    /*
     * Called from within Tasks::work() to process usb OUT data on the
     * main thread.
     */

    for (;;) {
        USBProtocolMsg m;
        m.len = UsbHardwareMC::instance().epReadPacket(0, m.bytes, m.bytesFree());

        if (m.len == 0) {
            break;
        }

        USBProtocol::dispatch(m);
    }
}

int UsbDevice::write(const uint8_t *buf, unsigned len, unsigned timeoutMillis)
{
    // XXX: support timeout
    return UsbHardwareMC::instance().epWritePacket(0, buf, len);
}
