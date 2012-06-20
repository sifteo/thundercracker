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
        WriteCommit
    };

    static void onUsbData(const USBProtocolMsg &m);

private:
    static FlashVolumeWriter writer;
    static uint32_t installationSize;
    static uint32_t installationProgress;
};

#endif // _USB_VOLUME_MANAGER_H
