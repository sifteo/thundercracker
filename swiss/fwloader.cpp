#include "fwloader.h"
#include "bootloader.h"
#include "aes128.h"
#include "macros.h"

#include "deployer.h"

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

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    uint32_t plainsz, crc;
    if (!checkFileDetails(f, plainsz, crc))
        return false;

    if (!bootloaderVersionIsCompatible())
        return false;

    resetWritePointer();

    if (!sendDetails(plainsz, crc)) {
        fprintf(stderr, "error sending details\n");
        return false;
    }

    if (!sendFirmwareFile(f)) {
        fprintf(stderr, "error sending file\n");
        return false;
    }

    fclose(f);

    while (dev.numPendingOUTPackets()) {
        dev.processEvents();
    }

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
 * Verify that this file at least has our Deployer's magic number,
 * and retrieve the CRC and size.
 */
bool FwLoader::checkFileDetails(FILE *f, uint32_t &plainsz, uint32_t &crc)
{
    // magic number is the file header
    fseek(f, 0L, SEEK_SET);
    uint64_t magic;
    unsigned numBytes = fread(&magic, 1, sizeof(magic), f);
    if (numBytes != sizeof(magic) || magic != Deployer::MAGIC) {
        fprintf(stderr, "magic number mismatch\n");
        return false;
    }

    // last 8 bytes are CRC and plaintext size
    fseek(f, -8L, SEEK_END);

    if (fread(&crc, 1, sizeof(crc), f) != sizeof(crc)) {
        fprintf(stderr, "couldn't read CRC\n");
        return false;
    }

    if (fread(&plainsz, 1, sizeof(plainsz), f) != sizeof(plainsz)) {
        fprintf(stderr, "couldn't read plainsz\n");
        return false;
    }

    return true;
}

/*
 * Load the given file to the bootloader.
 * The file should already be encrypted and in the final form that it will reside
 * in the STM32's flash.
 */
bool FwLoader::sendFirmwareFile(FILE *f)
{
    fseek(f, 0L, SEEK_END);
    unsigned extraBytes = sizeof(uint64_t) + (2 * sizeof(uint32_t));
    int numEncryptedBytes = ftell(f) - extraBytes;
    const unsigned filesz = numEncryptedBytes;

    // must be aes block aligned
    if (numEncryptedBytes & 0xf) {
        fprintf(stderr, "incorrect input format\n");
        return false;
    }

    // encrpyted data starts after the magic number
    fseek(f, sizeof(uint64_t), SEEK_SET);
    unsigned percent = 0;
    unsigned progress = 0;

    uint8_t usbBuf[IODevice::MAX_EP_SIZE] = { Bootloader::CmdWriteMemory };

    /*
     * Payload should be back to back AES128::BLOCK_SIZE chunks.
     */
    while (numEncryptedBytes > 0) {

        const unsigned payload = dev.maxOUTPacketSize() - 1;
        const unsigned chunk = (payload / AES128::BLOCK_SIZE) * AES128::BLOCK_SIZE;
        const int numBytes = fread(usbBuf + 1, 1, chunk, f);
        if (numBytes != chunk)
            break;

        dev.writePacket(usbBuf, numBytes + 1);
        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            dev.processEvents();

        progress += numBytes;
        numEncryptedBytes -= numBytes;

        const unsigned progressPercent = ((float)progress / (float)filesz) * 100;
        if (progressPercent != percent) {
            percent = progressPercent;
            fprintf(stderr, "progress: %d%%\n", percent);
            fflush(stderr);
        }
    }

    return true;
}

bool FwLoader::sendDetails(uint32_t size, uint32_t crc)
{
    /*
     * Finalize the transfer - specify our version of the crc and the file size.
     */
    uint8_t usbBuf[IODevice::MAX_EP_SIZE] = { Bootloader::CmdWriteDetails };
    *reinterpret_cast<uint32_t*>(&usbBuf[1]) = crc;
    *reinterpret_cast<uint32_t*>(&usbBuf[5]) = size;
    dev.writePacket(usbBuf, 9);
    return true;
}
