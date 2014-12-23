/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "fwloader.h"
#include "bootloader.h"
#include "aes128.h"
#include "macros.h"
#include "usbprotocol.h"
#include "deployer.h"
#include "progressbar.h"
#include "swisserror.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


int FwLoader::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2) {
        return EINVAL;
    }

    bool init = false;
    bool rpc = false;
    const char *path = NULL;
    unsigned int devicePID = IODevice::BASE_PID;
    unsigned int bootloaderPID = IODevice::BOOTLOADER_PID;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--pid") && i+1 < argc) {
            devicePID = strtoul(argv[i+1], NULL, 0);
            bootloaderPID = devicePID;
            i++;
        } else if (!strcmp(argv[i], "--init")) {
            init = true;
        } else if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "incorrect args\n");
            return EINVAL;
        }
    }

    FwLoader loader(_dev, rpc);

    if (init) {
        return loader.requestBootloaderUpdate(devicePID);
    }

    return loader.load(path, bootloaderPID);
}

FwLoader::FwLoader(IODevice &_dev, bool rpc) :
    dev(_dev), isRPC(rpc)
{
}

int FwLoader::requestBootloaderUpdate(unsigned int pid)
{
    if (!dev.open(IODevice::SIFTEO_VID, pid)) {
        fprintf(stderr, "Note: If the red LED is illuminated, `swiss update --init` is not required.\n");
        return ENODEV;
    }

    USBProtocolMsg m(USBProtocol::FactoryTest);
    m.append(10);   // bootload update request command
    if (dev.writePacket(m.bytes, m.len) < 0) {
        return EIO;
    }

    return EOK;
}

int FwLoader::load(const char *path, unsigned int pid)
{
    if (!dev.open(IODevice::SIFTEO_VID, pid)) {
        fprintf(stderr, "Note: Please ensure your device is in update mode, with the red LED illuminated\n");
        return ENODEV;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return ENOENT;
    }

    uint64_t magic;
    if (fread(&magic, sizeof(magic), 1, f) != 1) {
        fprintf(stderr, "magic number mismatch\n");
        return EIO;
    }

    int rv;

    if (magic == Deployer::MAGIC) {
        rv = loadSingle(f);
    } else if (magic == Deployer::MAGIC_CONTAINER) {
        rv = loadContainer(f);
    } else {
        fprintf(stderr, "invalid file format\n");
        rv = EINVAL;
    }

    fclose(f);

    return rv;
}

int FwLoader::loadSingle(FILE *f)
{
    long pos = ftell(f);

    fseek(f, 0L, SEEK_END);
    long endpos = ftell(f);     // determine file length

    fseek(f, pos, SEEK_SET);    // reset

    unsigned swVersion, hwVersion;
    if (!bootloaderVersionIsCompatible(swVersion, hwVersion)) {
        return EIO;
    }

    /*
     * This file doesn't specify which hardware version it was built for.
     * Because we can't guarantee the version of the hardware, we decline
     * to install on anything other than the default HW rev.
     *
     * This obviously doesn't address the case in which somebody builds
     * for a new HW rev but packages it in an old deploy format,
     * but maybe it helps prevent a few mixups.
     */

    if (hwVersion != DEFAULT_HW_VERSION) {
        fprintf(stderr, "error: attempting to install untagged firmware on non-default hardware\n");
        return EIO;
    }

    if (!installFile(f, endpos - pos)) {
        return EIO;
    }

    return EOK;
}

int FwLoader::loadContainer(FILE *f)
{
    unsigned swVersion, hwVersion;
    if (!bootloaderVersionIsCompatible(swVersion, hwVersion)) {
        return EIO;
    }

    uint32_t fileFormatVersion;
    if (fread(&fileFormatVersion, sizeof fileFormatVersion, 1, f) != 1) {
        return EIO;
    }

    for (;;) {

        Header hdr;
        if (fread(&hdr, sizeof hdr, 1, f) != 1) {
            if (!feof(f)) {
                fprintf(stderr, "fin read err\n");
                return EIO;
            }

            fprintf(stderr, "no image found for bootloader HW Rev %d\n", hwVersion);
            return EINVAL;
        }

        // look for a firmware binary that matches this base's hwVersion
        // and try to install it
        switch (hdr.key) {

        case Deployer::FirmwareBinary: {

            uint32_t hwRev, fwSize;
            if (!readFirmwareBinaryHeader(f, hwRev, fwSize)) {
                return EIO;
            }

            // found a valid hardware rev?
            if (hwRev == hwVersion) {
                return installFile(f, fwSize) ? EOK : EIO;
            }

            fseek(f, fwSize, SEEK_CUR); // skip data we're not interested in
            break;
        }

        default:
            // skip past data we're not interested in
            fseek(f, hdr.size, SEEK_CUR);
            break;
        }
    }

    return EINVAL;  // didn't find a file to install
}

bool FwLoader::installFile(FILE *f, unsigned fileSize)
{
    // store current file position while we retrieve
    // the crc and size from the end of the file details
    long pos = ftell(f);

    uint32_t plainsz, crc;
    if (!checkFileDetails(f, plainsz, crc, pos + fileSize)) {
        return false;
    }

    fseek(f, pos, SEEK_SET);    // reset

    resetBootloader();

    unsigned bytesToSend = fileSize - (2 * sizeof(uint32_t));
    if (!sendFirmwareFile(f, crc, plainsz, bytesToSend)) {
        fprintf(stderr, "error sending file\n");
        return false;
    }

    return true;
}

bool FwLoader::readFirmwareBinaryHeader(FILE *f, uint32_t &hwVersion, uint32_t &fwSize)
{
    /*
     * We're at the start of a FirmwareBinary section.
     * Capture the hw rev and the size of the firmware section,
     * leaving the file pointer at the beginning of the FW blob.
     */

    Header hwRevHdr;
    if (fread(&hwRevHdr, sizeof hwRevHdr, 1, f) != 1) {
        return false;
    }

    if (hwRevHdr.key != Deployer::HardwareRev) {
        return false;
    }

    if (fread(&hwVersion, sizeof hwVersion, 1, f) != 1) {
        return false;
    }

    Header fwBlobHdr;
    if (fread(&fwBlobHdr, sizeof fwBlobHdr, 1, f) != 1) {
        return false;
    }

    if (fwBlobHdr.key != Deployer::FirmwareBlob) {
        return false;
    }

    fwSize = fwBlobHdr.size;

    return true;
}

/*
 * Query the bootloader's version, and make sure we're compatible with it.
 */
bool FwLoader::bootloaderVersionIsCompatible(unsigned &swVersion, unsigned &hwVersion)
{
    const uint8_t versionRequest[] = { Bootloader::CmdGetVersion };
    dev.writePacket(versionRequest, sizeof versionRequest);

    while (!dev.numPendingINPackets())
        dev.processEvents(1);

    uint8_t usbBuf[IODevice::MAX_EP_SIZE];
    unsigned numBytes;
    dev.readPacket(usbBuf, sizeof usbBuf, numBytes);
    if (numBytes < 2 || usbBuf[0] != Bootloader::CmdGetVersion)
        return false;

    swVersion = usbBuf[1];
    // older bootloaders don't send the hardware version,
    // so check the length again to be sure
    hwVersion = (numBytes >= 3) ? usbBuf[2] : DEFAULT_HW_VERSION;

    return ((VERSION_COMPAT_MIN <= swVersion) && (swVersion <= VERSION_COMPAT_MAX));
}

void FwLoader::resetBootloader()
{
    const uint8_t ptrRequest[] = { Bootloader::CmdResetAddrPtr };
    dev.writePacket(ptrRequest, sizeof ptrRequest);

    while (dev.numPendingOUTPackets())
        dev.processEvents(1);
}

/*
 * Verify that this file at least has our Deployer's magic number,
 * and retrieve the CRC and size.
 */
bool FwLoader::checkFileDetails(FILE *f, uint32_t &plainsz, uint32_t &crc, long fileEnd)
{
    // last 8 bytes are CRC and plaintext size
    fseek(f, fileEnd - (2 * sizeof(uint32_t)), SEEK_SET);

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
bool FwLoader::sendFirmwareFile(FILE *f, uint32_t crc, uint32_t plaintextSize, unsigned fileSize)
{
    /*
     * initialBytes is the entire encrypted file, minus the the final block,
     * which is sent separately.
     */
    unsigned initialBytesToSend = fileSize - AES128::BLOCK_SIZE;

    // must be aes block aligned
    if (initialBytesToSend & 0xf) {
        fprintf(stderr, "incorrect input format\n");
        return false;
    }

    unsigned progress = 0;
    ScopedProgressBar progressBar(initialBytesToSend);

    /*
     * Payload should be back to back AES128::BLOCK_SIZE chunks.
     */
    while (initialBytesToSend) {

        uint8_t usbBuf[IODevice::MAX_EP_SIZE] = { Bootloader::CmdWriteMemory };
        const unsigned payload = MIN(dev.maxOUTPacketSize() - 1, initialBytesToSend);
        const unsigned chunk = (payload / AES128::BLOCK_SIZE) * AES128::BLOCK_SIZE;
        const unsigned numBytes = fread(usbBuf + 1, 1, chunk, f);
        if (numBytes != chunk) {
            return false;
        }

        if (dev.writePacket(usbBuf, numBytes + 1) < 0) {
            return false;
        }

        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS) {
            dev.processEvents(1);
        }

        progress += numBytes;
        initialBytesToSend -= numBytes;

        progressBar.update(progress);
        if (isRPC) {
            fprintf(stdout, "::progress:%u:%u\n", progress, fileSize - AES128::BLOCK_SIZE);
            fflush(stdout);
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
    unsigned numBytes = fread(p, 1, AES128::BLOCK_SIZE, f);
    p += AES128::BLOCK_SIZE;
    if (numBytes != AES128::BLOCK_SIZE)
        return false;

    memcpy(p, &crc, sizeof crc);
    p += sizeof(crc);

    memcpy(p, &plaintextSize, sizeof(plaintextSize));
    p += sizeof(plaintextSize);

    if (dev.writePacket(finalBuf, p - finalBuf) < 0) {
        return false;
    }

    while (dev.numPendingOUTPackets())
        dev.processEvents(1);

    return true;
}
