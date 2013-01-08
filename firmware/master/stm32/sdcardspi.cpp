#include "sdcardspi.h"
#include "mmcsd.h"

// Table for CRC-7 (polynomial x^7 + x^3 + 1)
const uint8_t SDCardSpi::crc7LUT[256] = {
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

void SDCardSpi::sendHeader(uint8_t cmd, uint32_t arg)
{
    // Wait for the bus to become idle if a write operation was in progress.
    //    wait(mmcp);

    uint8_t buf[6] = {
        0x40 | cmd,
        arg >> 24,
        arg >> 16,
        arg >> 8,
        arg
    };

    uint8_t crc = crc7(0, buf, 5);
    buf[5] = ((crc & 0x7F) << 1) | 0x01;    // shift to correct position and add stop bit

    spi.txDma(buf, sizeof buf);
    // XXX: wait for DMA
}

uint8_t SDCardSpi::receiveR1()
{
    for (unsigned i = 0; i < 9; i++) {
        uint8_t r1 = spi.transfer(0xff);
        if (r1 != 0xFF) {
            return r1;
        }
    }

    return 0xFF;
}

uint8_t SDCardSpi::receiveR3(uint8_t* buf)
{
    uint8_t r1 = receiveR1();
    spi.transferDma(buf, buf, 4);
    // XXX: wait for DMA

    return r1;
}

uint8_t SDCardSpi::sendCommandR1(uint8_t cmd, uint32_t arg)
{
    spi.begin();
    sendHeader(cmd, arg);
    uint8_t r1 = receiveR1();
    spi.end();

    return r1;
}

uint8_t SDCardSpi::sendCommandR3(uint8_t cmd, uint32_t arg, uint8_t *response)
{
    spi.begin();
    sendHeader(cmd, arg);
    uint8_t r1 = receiveR3(response);
    spi.end();

    return r1;
}

bool SDCardSpi::readCxD(uint8_t cmd, uint32_t cxd[4])
{
    spi.begin();
    sendHeader(cmd, 0);
    if (receiveR1() != 0) {
        spi.end();
        return false;
    }

    // Wait for data availability.
    for (unsigned i = 0; i < WAIT_DATA; i++) {
        uint8_t b = spi.transfer(0xff);
        if (b == 0xFE) {

            uint8_t buf[16 + 2];
            spi.transferDma(buf, buf, sizeof buf);
            // XXX: wait for DMA

            uint8_t *bp = buf;

            for (int i = 3; i >= 0; --i) {
                cxd[i] = uint32_t((bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | bp[3]);
                bp += 4;
            }

            // last 2 bytes are CRC - ignore for now
            spi.end();

            return true;
        }
    }

    return false;
}

bool SDCardSpi::connect()
{
    uint8_t r3[4];

    // Slow clock mode and 128 clock pulses.
//    spi.init();

    uint8_t dummy[16];
    spi.transferDma(dummy, dummy, sizeof dummy);

    // SPI mode selection.
    unsigned i = 0;
    for (;;) {
        if (sendCommandR1(MMCSD::CmdGoIdleState, 0) == 0x01) {
            break;
        }

        if (++i >= CMD0_RETRY) {
            goto failed;
        }

        // delay here?
    }

    /*
     * Try to detect if this is a high capacity card and switch to block
     * addresses if possible.
     *
     * This method is based on "How to support SDC Ver2 and high capacity cards"
     * by ElmChan.
     */
    if (sendCommandR3(MMCSD::CmdSendIfCond, MMCSD::Cmd8Pattern, r3) != 0x05) {

        // Switch to SDHC mode.
        for (i = 0;;) {
            if ((sendCommandR1(MMCSD::CmdAppCmd, 0) == 0x01) &&
                (sendCommandR3(MMCSD::CmdAppOpCond, 0x400001aa, r3) == 0x00))
            {
                break;
            }

            if (++i >= ACMD41_RETRY) {
                goto failed;
            }

            // delay here?
        }

        // Execute dedicated read on OCR register
        sendCommandR3(MMCSD::CmdReadOCR, 0, r3);

        // Check if CCS is set in response. Card operates in block mode if set.
        if (r3[0] & 0x40) {
            // store this
        }
    }

    // Initialization.
    i = 0;
    for (;;) {

        uint8_t b = sendCommandR1(MMCSD::CmdInit, 0);
        if (b == 0x00) {
            break;
        }

        if (b != 0x01) {
            goto failed;
        }

        if (++i >= CMD1_RETRY) {
            goto failed;
        }

        // delay here?
    }

    // Initialization complete, full speed.
//    spi.init();

    // Setting block size.
    if (sendCommandR1(MMCSD::CmdSetBlocklen, MMCSD::BlockSize) != 0x00) {
        goto failed;
    }

    // Determine capacity.
    if (readCxD(MMCSD::CmdSendCSD, csd)) {
        goto failed;
    }

    capacity = getCapacity(csd);
    if (capacity == 0) {
        goto failed;
    }

    if (readCxD(MMCSD::CmdSendCID, cid)) {
        goto failed;
    }

    return true;

failed:
    // handle error, deinit spi?
    return false;
}

bool SDCardSpi::read(uint32_t startblk, uint8_t *buf, unsigned numBlocks)
{
    spi.begin();
    sendHeader(MMCSD::CmdReadMultiBlock, startblk);

    if (receiveR1() != MMCSD::R1Success) {
        // error. deinit spi?
        spi.end();
        return false;
    }


    /*
     * Main read loop
     */

    while (numBlocks) {

        for (unsigned i = 0; i < WAIT_DATA; i++) {
            if (!readBlock(buf, MMCSD::BlockSize)) {
                return false;
            }
        }

        buf += MMCSD::BlockSize;
        numBlocks--;
    }

    /*
     * Stop reading
     */

    static const uint8_t stopcmd[] = { 0x40 | MMCSD::CmdStopTransmission,
                                       0, 0, 0, 0, 1, 0xFF };

    spi.txDma(stopcmd, sizeof stopcmd);
    // XXX: wait for DMA

    uint8_t result = receiveR1();
    if (result != MMCSD::R1Success) {
        // TODO: handle error
    }

    spi.end();

    return true;
}

bool SDCardSpi::readBlock(uint8_t *buf, unsigned len)
{
    /*
     * Helper to read a single chunk of size len
     */

    uint8_t b = spi.transfer(0xff);
    if (b == 0xFE) {
        spi.transferDma(buf, buf, len);
        // XXX: wait for DMA

        uint8_t crc[2];
        spi.transferDma(crc, crc, sizeof crc);
        // XXX: wait for DMA

        // do anything with CRC?
        return true;
    }

    // Timeout - handle error. deinit spi?
    spi.end();
    return false;
}

bool SDCardSpi::write(uint32_t startblk, const uint8_t *buf, unsigned len)
{
    spi.begin();
    sendHeader(MMCSD::CmdWriteMultiBlock, startblk);

    if (receiveR1() != MMCSD::R1Success) {
        spi.end();
        //    spiStop(mmcp->config->spip);
        return false;
    }

    static const uint8_t start[] = { 0xFF, 0xFC };
    spi.txDma(start, sizeof start);
    // XXX: wait for DMA

    spi.txDma(buf, MMCSD::BlockSize);
    // XXX: wait for DMA

    uint8_t b[3];
    spi.transferDma(b, b, sizeof b);
    // first 2 bytes are CRC - we ignore. last is status
    // wait for DMA

    if ((b[2] & 0x1F) != MMCSD::DataAccepted) {
        // Handle error. deinit spi?
        spi.end();
        return false;
    }

    static const uint8_t stop[] = { 0xFD, 0xFF };
    spi.txDma(stop, sizeof stop);
    // XXX: wait for DMA
    spi.end();

    return true;
}

bool SDCardSpi::erase(uint32_t startblk, uint32_t endblk)
{
    if (!sendCommandR1(MMCSD::CmdEraseRWBlockStart, startblk)) {
        goto failed;
    }

    if (!sendCommandR1(MMCSD::CmdEraseRWBlockEnd, endblk)) {
        goto failed;
    }

    if (!sendCommandR1(MMCSD::CmdErase, 0)) {
        goto failed;
    }

    return true;

failed:
    // handle error, deinit spi?
    return false;
}

uint32_t SDCardSpi::getCapacity(uint32_t csd[4])
{
    switch (csd[3] >> 30) {
    uint32_t a, b, c;
    case 0: // CSD version 1.0
        a = getSlice(csd, MMCSD_CSD_10_C_SIZE_SLICE);
        b = getSlice(csd, MMCSD_CSD_10_C_SIZE_MULT_SLICE);
        c = getSlice(csd, MMCSD_CSD_10_READ_BL_LEN_SLICE);
        return (a + 1) << (b + 2) << (c - 9);       // 2^9 == MMCSD::BlockSize.

    case 1: // CSD version 2.0.
        return 1024 * (getSlice(csd, MMCSD_CSD_20_C_SIZE_SLICE) + 1);

    default:
        return 0;   // Reserved value detected.
    }
}

uint32_t SDCardSpi::getSlice(uint32_t *data, uint32_t end, uint32_t start)
{
    unsigned startidx, endidx, startoff;
    uint32_t endmask;

    startidx = start / 32;
    startoff = start % 32;
    endidx   = end / 32;
    endmask  = (1 << ((end % 32) + 1)) - 1;

    // One or two pieces?
    if (startidx < endidx) {
        return (data[startidx] >> startoff) | ((data[endidx] & endmask) << (32 - startoff));
    }

    return (data[startidx] & endmask) >> startoff;
}
