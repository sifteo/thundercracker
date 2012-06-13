#include "loader.h"
#include "usbdevice.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

Loader::Loader()
{
}

bool Loader::load(const char *path, int vid, int pid)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    UsbDevice dev;
    if (!dev.open(vid, pid)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    const uint8_t versionRequest[] = { CmdGetVersion };
    dev.writePacket(versionRequest, sizeof versionRequest);

    while (!dev.numPendingINPackets())
        UsbDevice::processEvents();

    uint8_t usbBuf[UsbDevice::MAX_EP_SIZE];
    unsigned numBytes = dev.readPacket(usbBuf, sizeof usbBuf);
    if (usbBuf[0] == CmdGetVersion && numBytes >= 2)
        fprintf(stderr, "bootloader version: %d\n", usbBuf[1]);
    else
        fprintf(stderr, "couldn't get bootloader version\n");



    fseek(f, 0L, SEEK_END);
    unsigned filesz = ftell(f);
    fseek(f, 0L, SEEK_SET);

    unsigned percent = 0;
    unsigned progress = 0;

    usbBuf[0] = CmdWriteMemory;

    while (!feof(f)) {

        int numBytes = fread(usbBuf + 1, 1, sizeof(usbBuf) - 1, f);
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

    dev.close();
    UsbDevice::processEvents();

    return true;
}
