#include "fwloader.h"
#include "bootloader.h"
#include "aes128.h"
#include "macros.h"
#include "usbprotocol.h"
#include "deployer.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>


int FwLoader::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2)
        return 1;


    bool success;
    bool init = false;
    bool rpc = false;
    const char *path = NULL;
    
    for (unsigned i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--init")) {
            init = true;
        } else if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "incorrect args\n");
            return 1;
        }
    }
    
    FwLoader loader(_dev, rpc);
    
    if (init) {
        success = loader.requestBootloaderUpdate();
    } else {
        success = loader.load(path);
    }

    while (_dev.numPendingOUTPackets()) {
        _dev.processEvents();
    }
    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

FwLoader::FwLoader(IODevice &_dev, bool rpc) :
    dev(_dev), isRPC(rpc)
{
}

bool FwLoader::requestBootloaderUpdate()
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached - if the red LED is illuminated, `swiss update --init` is not required.\n");
        return false;
    }

    USBProtocolMsg m(USBProtocol::FactoryTest);
    m.append(10);   // bootload update request command
    dev.writePacket(m.bytes, m.len);
    return true;
}

bool FwLoader::load(const char *path)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BOOTLOADER_PID)) {
        fprintf(stderr, "device is not attached - please ensure your device is in update mode, with the red LED illuminated\n");
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

    resetBootloader();

    if (!sendFirmwareFile(f, crc, plainsz)) {
        fprintf(stderr, "error sending file\n");
        return false;
    }

    fclose(f);

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

void FwLoader::resetBootloader()
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
bool FwLoader::sendFirmwareFile(FILE *f, uint32_t crc, uint32_t size)
{
    fseek(f, 0L, SEEK_END);
    unsigned extraBytes = sizeof(uint64_t) + (2 * sizeof(uint32_t));
    const unsigned filesz = ftell(f) - extraBytes;

    /*
     * initialBytes is the entire encrypted file, minus the the final block,
     * which is sent separately.
     */
    int initialBytesToSend = filesz - AES128::BLOCK_SIZE;

    // must be aes block aligned
    if (initialBytesToSend & 0xf) {
        fprintf(stderr, "incorrect input format\n");
        return false;
    }

    // encrpyted data starts after the magic number
    fseek(f, sizeof(uint64_t), SEEK_SET);
    unsigned percent = 0;
    unsigned progress = 0;

    /*
     * Payload should be back to back AES128::BLOCK_SIZE chunks.
     */
    while (initialBytesToSend) {

        uint8_t usbBuf[IODevice::MAX_EP_SIZE] = { Bootloader::CmdWriteMemory };
        const unsigned payload = MIN(dev.maxOUTPacketSize() - 1, initialBytesToSend);
        const unsigned chunk = (payload / AES128::BLOCK_SIZE) * AES128::BLOCK_SIZE;
        const int numBytes = fread(usbBuf + 1, 1, chunk, f);
        if (numBytes != chunk)
            return false;

        dev.writePacket(usbBuf, numBytes + 1);
        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            dev.processEvents();

        progress += numBytes;
        initialBytesToSend -= numBytes;

        const unsigned progressPercent = ((float)progress / (float)filesz) * 100;
        if (progressPercent != percent) {
            percent = progressPercent;
            fprintf(stderr, "progress: %d%%\n", percent);
            fflush(stderr);
            if (isRPC) {
                fprintf(stdout, "::progress:%u:%u\n", percent, 100); fflush(stdout);
            }
        }
    }

    /*
     * Write the final block.
     * Decrypter needs to treat this sepcially in order to deal with the padding
     * at the end, and also gives us a chance to finalize the update by sending
     * the expected CRC and size for the plaintext.
     */

    ASSERT(initialBytesToSend == 0);

    uint8_t finalBuf[IODevice::MAX_EP_SIZE] = { Bootloader::CmdWriteFinal };

    uint8_t *p = finalBuf + 1;
    int numBytes = fread(p, 1, AES128::BLOCK_SIZE, f);
    p += AES128::BLOCK_SIZE;
    if (numBytes != AES128::BLOCK_SIZE)
        return false;

    memcpy(p, &crc, sizeof crc);
    p += sizeof(crc);

    memcpy(p, &size, sizeof(size));
    p += sizeof(size);

    dev.writePacket(finalBuf, p - finalBuf);

    return true;
}
