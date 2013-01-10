#include "sdcardspi.h"
#include "tasks.h"
#include "systime.h"

volatile bool SDCardSpi::dmaInProgress;

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
    waitForNotBusy(0xff);

    uint8_t buf[6] = {
        0x40 | cmd,
        arg >> 24,
        arg >> 16,
        arg >> 8,
        arg
    };

    uint8_t crc = crc7(0, buf, 5);
    buf[5] = ((crc & 0x7F) << 1) | 0x01;    // shift to correct position and add stop bit

    dmaInProgress = true;
    spi.txDma(buf, sizeof buf);
    waitForDma();
}

uint8_t SDCardSpi::receiveR1()
{
    return waitForNotBusy(0xff);
}

uint8_t SDCardSpi::receiveR3(MMCSD::R7 &response)
{
    dmaInProgress = true;
    response.bytes[0] = receiveR1();
    spi.transferDma(&response.bytes[1], &response.bytes[1], sizeof(response.bytes) - 1);
    waitForDma();

    return response.r1();
}

uint8_t SDCardSpi::sendCommandR1(uint8_t cmd, uint32_t arg)
{
    spi.begin();
    sendHeader(cmd, arg);
    uint8_t r1 = receiveR1();
    spi.end();

    return r1;
}

uint8_t SDCardSpi::sendCommandR3(uint8_t cmd, uint32_t arg, MMCSD::R7 &response)
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

            uint8_t buf[16 + 2];    // 16 bytes of data and 2 for CRC. CRC is ignored for now.
            dmaInProgress = true;
            spi.transferDma(buf, buf, sizeof buf);
            waitForDma();

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
    // Slow clock mode during init, must be between 100kHz and 400kHz
    const SPIMaster::Config cfgSlow = {
        Dma::VeryHighPrio,
        SPIMaster::fPCLK_128    // 36MHz / 128 == 281.250kHz
    };

    // full clock mode
    const SPIMaster::Config cfgFull = {
        Dma::VeryHighPrio,
        SPIMaster::fPCLK_2      // 36MHz / 2 == 18MHz
    };

    spi.init(cfgSlow);

    // ensure we wait 128 clock pulses, chip select is still high
    uint8_t clockPulses[128 / 8];
    dmaInProgress = true;

    spi.end();
    spi.transferDma(clockPulses, clockPulses, sizeof clockPulses);
    waitForDma();

    // SPI mode selection.
    for (unsigned i = 0;;) {
        uint8_t r1temp = sendCommandR1(MMCSD::CmdGoIdleState, 0);

        if (r1temp == 0x01) {
            break;
        }

        if (++i >= CMD0_RETRY) {
            // XXX: disabling for now, since init is only working when my SC card is
            //      powered up while the MCU is already running...this lets us idle till then
//            goto failed;
        }

        delayMillis(10);
    }

    /*
     * Try to detect if this is a high capacity card and switch to block
     * addresses if possible.
     *
     * This method is based on "How to support SDC Ver2 and high capacity cards"
     * by ElmChan.
     */

    MMCSD::R7 res;
    if (sendCommandR3(MMCSD::CmdSendIfCond, MMCSD::Cmd8Pattern, res) != 0x05) {

        for (unsigned i = 0;;) {
            // set HCS to indicate that we're SDHC/SDXC friendly
            uint8_t appCmd = sendCommandR1(MMCSD::CmdAppCmd, 0);
            uint8_t appopcondR1 = sendCommandR3(MMCSD::CmdAppOpCond, 0x400001aa, res);
            if (appCmd == 0x01 && appopcondR1 == 0x00) {
                break;
            }

            if (++i >= ACMD41_RETRY) {
                goto failed;
            }

            delayMillis(10);
        }

        // Execute dedicated read on OCR register
        if (sendCommandR3(MMCSD::CmdReadOCR, 0, res) != 0x00) {
            goto failed;
        }

        // Check if CCS is set in response. Card operates in block mode if set.
        if (res.cmdVersion() & 0x40) {
            // store this
        } else {

        }
    }

    // Initialization.
    for (unsigned i = 0;;) {

        uint8_t b = sendCommandR1(MMCSD::CmdInit, 0);
        if (b == 0x00) {
            break;
        }

        // the only valid incomplete result is Idle
        if (b != 0x01) {
            goto failed;
        }

        if (++i >= CMD1_RETRY) {
            goto failed;
        }

        delayMillis(10);
    }

    // Initialization complete, full speed.
    spi.init(cfgFull);

    // Setting block size.
    if (sendCommandR1(MMCSD::CmdSetBlocklen, MMCSD::BlockSize) != 0x00) {
        UART("CmdSetBlocklen failed\r\n");
        goto failed;
    }

    // Determine capacity.
    if (!readCxD(MMCSD::CmdSendCSD, csd)) {
        UART("CmdSendCSD failed\r\n");
        goto failed;
    }

    capacity = getCapacity(csd);
    if (capacity == 0) {
        UART("getCapacity failed\r\n");
        goto failed;
    }

    if (!readCxD(MMCSD::CmdSendCID, cid)) {
        UART("CmdSendCID failed\r\n");
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
    waitForDma();

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
        waitForDma();

        uint8_t crc[2];
        spi.transferDma(crc, crc, sizeof crc);
        waitForDma();

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
        // deinit spi?
        return false;
    }

    static const uint8_t start[] = { 0xFF, 0xFC };
    spi.txDma(start, sizeof start);
    waitForDma();

    spi.txDma(buf, MMCSD::BlockSize);
    waitForDma();

    uint8_t b[3];
    spi.transferDma(b, b, sizeof b);
    // first 2 bytes are CRC - we ignore. last is status
    waitForDma();

    if ((b[2] & 0x1F) != MMCSD::DataAccepted) {
        // Handle error. deinit spi?
        spi.end();
        return false;
    }

    static const uint8_t stop[] = { 0xFD, 0xFF };
    spi.txDma(stop, sizeof stop);
    waitForDma();
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
    unsigned startidx = start / 32;
    unsigned startoff = start % 32;
    unsigned endidx   = end / 32;
    uint32_t endmask  = (1 << ((end % 32) + 1)) - 1;

    // One or two pieces?
    if (startidx < endidx) {
        return (data[startidx] >> startoff) | ((data[endidx] & endmask) << (32 - startoff));
    }

    return (data[startidx] & endmask) >> startoff;
}

void SDCardSpi::dmaCompletionCallback()
{
    dmaInProgress = false;
}

bool SDCardSpi::waitForDma()
{
    while (dmaInProgress) {
        Tasks::waitForInterrupt();
    }

    return true;
}

uint8_t SDCardSpi::waitForNotBusy(unsigned tries)
{
    /*
     * While busy, the SD Card will reply with 0xff.
     *
     * XXX: there appears to be no better way to detect the busy
     *      state of the card, but this should at least be interrupt
     *      driven on a per-byte basis.
     */

    uint8_t r = 0xff;
    while (tries-- && r == 0xff) {
        r = spi.transfer(0xff);
    }

    return r;
}

void SDCardSpi::delayMillis(unsigned ms)
{
    /*
     * terrible helper to deal with SC busy-ness.
     */

    SysTime::Ticks deadline = SysTime::ticks() + SysTime::msTicks(ms);
    while (SysTime::ticks() < deadline) {
        Tasks::idle();
        Tasks::resetWatchdog(); // temp
    }
}
