#ifndef _USB_VOLUME_MANAGER_H
#define _USB_VOLUME_MANAGER_H

#include "usbprotocol.h"
#include "flash_volume.h"

class UsbVolumeManager
{
public:
    enum Command {
        WriteGameHeader,
        WritePayload,
        WriteCommit,
        WroteHeaderOK,
        WroteHeaderFail,
        WriteLauncherHeader,
        VolumeOverview,
        VolumeDetail,
        VolumeMetadata,
        DeleteEverything,
        DeleteVolume,
        DeleteSysLFS,
        FirmwareVersion,
    };

    struct VolumeOverviewReply {
        unsigned systemBytes;
        unsigned freeBytes;
        BitVector<256> bits;
    };

    struct VolumeDetailReply {
        unsigned selfBytes;
        unsigned childBytes;
        uint16_t type;
    };

    struct VolumeMetadataRequest {
        unsigned volume;
        unsigned key;
    };

    static void onUsbData(const USBProtocolMsg &m);

private:
    static FlashVolumeWriter writer;
};

#endif // _USB_VOLUME_MANAGER_H
