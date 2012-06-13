#include "fwloader.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

FwLoader::FwLoader()
{
}

int FwLoader::run(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    const unsigned vid = 0x22fa;
    const unsigned pid = 0x0115;

    FwLoader loader;
    bool success = loader.load(argv[1], vid, pid);
    return success ? 0 : 1;
}

bool FwLoader::load(const char *path, int vid, int pid)
{
    if (!dev.open(vid, pid)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    if (!bootloaderVersionIsCompatible())
        return false;

    if (!sendFirmwareFile(path))
        return false;

    dev.close();
    UsbDevice::processEvents();

    return true;
}

/*
 * Query the bootloader's version, and make sure we're compatible with it.
 */
bool FwLoader::bootloaderVersionIsCompatible()
{
    const uint8_t versionRequest[] = { CmdGetVersion };
    dev.writePacket(versionRequest, sizeof versionRequest);

    while (!dev.numPendingINPackets())
        UsbDevice::processEvents();

    uint8_t usbBuf[UsbDevice::MAX_EP_SIZE];
    unsigned numBytes = dev.readPacket(usbBuf, sizeof usbBuf);
    if (numBytes < 2 || usbBuf[0] != CmdGetVersion)
        return false;

    unsigned version = usbBuf[1];
    return ((VERSION_COMPAT_MIN <= version) && (version <= VERSION_COMPAT_MAX));
}

/*
 * Load the given file to the bootloader.
 * The file should already be encrypted and in the final form that it will reside
 * in the STM32's flash.
 */
bool FwLoader::sendFirmwareFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    fseek(f, 0L, SEEK_END);
    unsigned filesz = ftell(f);
    fseek(f, 0L, SEEK_SET);

    unsigned percent = 0;
    unsigned progress = 0;

    uint8_t usbBuf[UsbDevice::MAX_EP_SIZE];
    usbBuf[0] = CmdWriteMemory;

    while (!feof(f)) {

        const unsigned chunk = dev.outEpMaxPacketSize() - 1;
        int numBytes = fread(usbBuf + 1, 1, chunk, f);
        if (numBytes <= 0)
            continue;

        dev.writePacket(usbBuf, numBytes + 1);
        while (dev.numPendingOUTPackets() > UsbDevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            UsbDevice::processEvents();

        progress += numBytes;
        const unsigned progressPercent = ((float)progress / (float)filesz) * 100;
        if (progressPercent != percent) {
            percent = progressPercent;
            fprintf(stderr, "progress: %d%%\n", percent);
            fflush(stderr);
        }
    }

    fclose(f);

    while (dev.numPendingOUTPackets()) {
        UsbDevice::processEvents();
    }
}
