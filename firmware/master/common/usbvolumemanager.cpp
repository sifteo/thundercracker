#include "usbvolumemanager.h"
#include "elfprogram.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#endif

FlashVolumeWriter UsbVolumeManager::writer;

void UsbVolumeManager::onUsbData(const USBProtocolMsg &m)
{
    switch (m.header & 0xff) {

    case WriteHeader: {

        if (m.payloadLen() < 5)
            break;

        const uint32_t numBytes = *reinterpret_cast<const uint32_t*>(m.payload);
        const char* packageStr = reinterpret_cast<const char*>(m.payload + sizeof(numBytes));
        if (!memchr(packageStr, 0, m.payloadLen() - 4))
            break;

        /*
         * Delete any previous versions of this game that are installed.
         */
        FlashVolumeIter vi;
        FlashVolume vol;

        vi.begin();
        while (vi.next(vol)) {
            if (vol.getType() != FlashVolume::T_GAME)
                continue;

            FlashBlockRef ref;
            Elf::Program program;
            if (program.init(vol.getPayload(ref))) {
                const char *str = program.getMetaString(ref, _SYS_METADATA_PACKAGE_STR);
                if (str && !strcmp(str, packageStr))
                    vol.deleteTree();
            }
        }

        USBProtocolMsg m(USBProtocol::Installer);
        if (writer.begin(FlashVolume::T_GAME, numBytes)) {
            m.header |= WroteHeaderOK;
        } else {
            UART("couldn't start new header\r\n");
            m.header |= WroteHeaderFail;
        }

#ifndef SIFTEO_SIMULATOR
        UsbDevice::write(m.bytes, m.len);
#endif
        break;
    }

    case WritePayload:
        writer.appendPayload(m.payload, m.payloadLen());
        break;

    case WriteCommit:
        writer.commit();
        break;

    default:
        break;
    }
}
