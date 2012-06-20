#include "usbvolumemanager.h"

FlashVolumeWriter UsbVolumeManager::writer;
uint32_t UsbVolumeManager::installationSize;
uint32_t UsbVolumeManager::installationProgress;

void UsbVolumeManager::onUsbData(const USBProtocolMsg &m)
{
    switch (m.header & 0xff) {

    case WriteHeader: {
        const uint32_t numBytes = *reinterpret_cast<const uint32_t*>(m.payload);
        if (m.payloadLen() < sizeof(numBytes)) {
            // send error?
            break;
        }

        installationSize = numBytes;
        installationProgress = 0;
        if (!writer.begin(FlashVolume::T_GAME, numBytes)) {
            // send error?
        }
        break;
    }

    case WritePayload:
        writer.appendPayload(m.payload, m.payloadLen());
        installationProgress += m.payloadLen();
        break;

    case WriteCommit:
        if (installationProgress == installationSize)
            writer.commit();
        break;

    default:
        break;
    }
}
