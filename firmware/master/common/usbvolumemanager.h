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
        DeleteReformat,
        FirmwareVersion,
        PairCube,
        PairingSlotDetail,
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

    struct PairCubeRequest {
        uint64_t hwid;
        unsigned pairingSlot;
    };

    struct PairingSlotDetailReply {
        uint64_t hwid;
        unsigned pairingSlot;
    };

    static void onUsbData(const USBProtocolMsg &m);

private:
    static FlashVolumeWriter writer;

    // handlers
    static ALWAYS_INLINE void volumeOverview(USBProtocolMsg &reply);
    static ALWAYS_INLINE void volumeDetail(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void volumeMetadata(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void pairCube(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void pairingSlotDetail(const USBProtocolMsg &m, USBProtocolMsg &reply);
};

#endif // _USB_VOLUME_MANAGER_H
