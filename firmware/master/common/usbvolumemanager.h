#ifndef _USB_VOLUME_MANAGER_H
#define _USB_VOLUME_MANAGER_H

#include "usbprotocol.h"
#include "flash_volume.h"

class UsbVolumeManager
{
public:
    enum Command {
        WriteHeader,
        WritePayload,
        WriteCommit,
        WroteHeaderOK,
        WroteHeaderFail,
    };

    static void onUsbData(const USBProtocolMsg &m);

private:
    static FlashVolumeWriter writer;
};

#endif // _USB_VOLUME_MANAGER_H
