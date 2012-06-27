#include "fwloader.h"
#include "bootloader.h"
#include "aes128.h"
#include "macros.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>


int FwLoader::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2)
        return 1;

    FwLoader loader(_dev);
    bool success = loader.load(argv[1], IODevice::SIFTEO_VID, IODevice::BOOTLOADER_PID);
    return success ? 0 : 1;
}

FwLoader::FwLoader(IODevice &_dev) :
    dev(_dev)
{
}

bool FwLoader::load(const char *path, int vid, int pid)
{
    if (!dev.open(vid, pid)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    if (!bootloaderVersionIsCompatible())
        return false;

    resetWritePointer();

    if (!sendFirmwareFile(path))
        return false;

    return true;
}

/*
 * Query the bootloader's version, and make sure we're compatible with it.
 */
bool FwLoader::bootloaderVersionIsCompatible()
{
    const uint8_t versionRequest[] = { Bootloader::CmdGetVersion };
    dev.writePacket(versionRequest, sizeof versionRequest);

    while (!dev.numPendingINPackets())
        dev.processEvents();

    uint8_t usbBuf[IODevice::MAX_EP_SIZE];
    unsigned numBytes = dev.readPacket(usbBuf, sizeof usbBuf);
    if (numBytes < 2 || usbBuf[0] != Bootloader::CmdGetVersion)
        return false;

    unsigned version = usbBuf[1];
    return ((VERSION_COMPAT_MIN <= version) && (version <= VERSION_COMPAT_MAX));
}

void FwLoader::resetWritePointer()
{
    const uint8_t ptrRequest[] = { Bootloader::CmdResetAddrPtr };
    dev.writePacket(ptrRequest, sizeof ptrRequest);

    while (!dev.numPendingOUTPackets())
        dev.processEvents();
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

    /*
     * Last 4 bytes are unencrypted CRC
     */
    fseek(f, -4L, SEEK_CUR);
    uint32_t crc;
    if (fread(&crc, 1, sizeof(uint32_t), f) != sizeof(uint32_t))
        return false;

    fprintf(stderr, "loading crc %lu\n", crc);
    fprintf(stderr, "sending filesz %lu\n", filesz);

    fseek(f, 0L, SEEK_SET);
    unsigned percent = 0;
    unsigned progress = 0;

    uint8_t usbBuf[IODevice::MAX_EP_SIZE];
    usbBuf[0] = Bootloader::CmdWriteMemory;

    while (!feof(f)) {

        /*
         * payload (everything after command byte) should be back to back
         * chunks of AES128::BLOCK_SIZE bytes
         */
        const unsigned payload = dev.maxOUTPacketSize() - 1;
        const unsigned chunk = (payload / AES128::BLOCK_SIZE) * AES128::BLOCK_SIZE;
        const int numBytes = fread(usbBuf + 1, 1, chunk, f);
        if (numBytes <= 0)
            continue;

        dev.writePacket(usbBuf, numBytes + 1);
        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            dev.processEvents();

        progress += numBytes;
        const unsigned progressPercent = ((float)progress / (float)filesz) * 100;
        if (progressPercent != percent) {
            percent = progressPercent;
            fprintf(stderr, "progress: %d%%\n", percent);
            fflush(stderr);
        }
    }

    /*
     * Finalize the transfer - specify our version of the crc and the file size.
     */
    usbBuf[0] = Bootloader::CmdWriteDetails;
    *reinterpret_cast<uint32_t*>(&usbBuf[1]) = crc;
    *reinterpret_cast<uint32_t*>(&usbBuf[5]) = filesz;
    dev.writePacket(usbBuf, 9);

    fclose(f);

    while (dev.numPendingOUTPackets()) {
        dev.processEvents();
    }

    return true;
}
