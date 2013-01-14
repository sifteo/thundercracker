#include "sdcardspi.h"
#include "tasks.h"
#include "systime.h"

#include "string.h"

volatile bool SDCardSpi::dmaInProgress;

bool SDCardSpi::select()
{
    spi.begin();
    spi.transfer(0xFF);     // Dummy clock (force DO enabled)

    return waitForIdle(0xfff);
}

uint8_t SDCardSpi::sendCommand(uint8_t cmd, uint32_t arg)
{
    // Is this an ACMD? Send a CMD55 first
    uint8_t r1;
    if (cmd & 0x80) {
        r1 = sendOneCommand(MMCSD::CmdAppCmd, 0);
        if (r1 > MMCSD::R1IdleState) {
            return r1;
        }
    }

    return sendOneCommand(cmd & 0x7F, arg);
}

uint8_t SDCardSpi::sendOneCommand(uint8_t cmd, uint32_t arg)
{
    /*
     * Should only be called from within sendCommand()
     */

    spi.end();
    select();

    uint8_t buf[6] = {
        0x40 | cmd,
        arg >> 24,
        arg >> 16,
        arg >> 8,
        arg
    };

    /*
     * Shortcut: we never enable CRC mode, so just hardcode the correct CRC
     * for the few commands that we use before we're initialized
     */
    switch (cmd) {
    case MMCSD::CmdGoIdleState: buf[5] = 0x95; break;
    case MMCSD::CmdSendIfCond:  buf[5] = 0X87; break;
    default:                    buf[5] = 0x01; break;   // stop bit for dummy CRC
    }

    dmaInProgress = true;
    spi.txDma(buf, sizeof buf);
    waitForDma();

    return waitForResponse(10);
}

void SDCardSpi::sendHeader(uint8_t cmd, uint32_t arg)
{
    waitForIdle(0xff);

    uint8_t buf[6] = {
        0x40 | cmd,
        arg >> 24,
        arg >> 16,
        arg >> 8,
        arg
    };

    /*
     * Shortcut: we never enable CRC mode, so just hardcode the  CRC
     * for the few commands that we use before we're initialized
     */
    switch (cmd) {
    case MMCSD::CmdGoIdleState: buf[5] = 0x95; break;
    case MMCSD::CmdSendIfCond:  buf[5] = 0X87; break;
    default:                    buf[5] = 0xff; break;
    }

    dmaInProgress = true;
    spi.txDma(buf, sizeof buf);
    waitForDma();
}

uint8_t SDCardSpi::receiveR1()
{
    return waitForResponse(0xff);
}

uint8_t SDCardSpi::receiveR3(MMCSD::R7 &response)
{
    dmaInProgress = true;
    response.bytes[0] = receiveR1();
    spi.transferDma(&response.bytes[1], &response.bytes[1], sizeof(response.bytes) - 1);
    waitForDma();

    return response.r1();
}

bool SDCardSpi::readCxD(uint8_t cmd, uint32_t cxd[4])
{
    spi.begin();
    if (sendCommand(cmd, 0) != 0) {
        spi.end();
        return false;
    }

    /*
     * Wait for data availability.
     * XXX: should at least be interrupt driven.
     */
    for (unsigned i = 0; i < WAIT_DATA; i++) {
        if (spi.transfer(0xff) == MMCSD::DataStartBlock) {

            uint8_t buf[16 + 2];    // 16 bytes of data and 2 for CRC
            dmaInProgress = true;
            spi.transferDma(buf, buf, sizeof buf);
            waitForDma();

            uint8_t *bp = buf;
            for (int i = 3; i >= 0; --i) {
                cxd[i] = uint32_t((bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | bp[3]);
                bp += 4;
            }
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

    spi.end();
    dmaInProgress = true;
    spi.transferDma(clockPulses, clockPulses, sizeof clockPulses);
    waitForDma();

    uint8_t r1;

    // SPI mode selection.
    for (unsigned i = 0;;) {

        r1 = sendCommand(MMCSD::CmdGoIdleState, 0);
        if (r1 == MMCSD::R1IdleState) {
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

    r1 = sendCommand(MMCSD::CmdSendIfCond, MMCSD::Cmd8Pattern);
    if (r1 != 0x05) {

        uint8_t r7[4];
        dmaInProgress = true;
        spi.transferDma(r7, r7, sizeof r7);
        waitForDma();

        for (unsigned i = 0;;) {
            // set HCS to indicate that we're SDHC/SDXC friendly
            if (sendCommand(MMCSD::CmdAppOpCond, 0x40000000) == MMCSD::R1Success) {
                break;
            }

            if (++i >= ACMD41_RETRY) {
                goto failed;
            }

            delayMillis(10);
        }

        // Execute dedicated read on OCR register
        if (sendCommand(MMCSD::CmdReadOCR, 0) != 0x00) {
            goto failed;
        }

        dmaInProgress = true;
        spi.transferDma(r7, r7, sizeof r7);
        waitForDma();

        // Check if CCS is set in response. Card operates in block mode if set.
        if (r7[0] & 0x40) {
            blockAddresses = true;
        }
    }

    // Initialization.
    for (unsigned i = 0;;) {

        r1 = sendCommand(MMCSD::CmdInit, 0);
        if (r1 == MMCSD::R1Success) {
            break;
        }

        // the only valid incomplete result is Idle
        if (r1 != MMCSD::R1IdleState) {
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
    if (sendCommand(MMCSD::CmdSetBlocklen, MMCSD::BlockSize) != 0x00) {
        goto failed;
    }

    // Determine capacity.
    if (!readCxD(MMCSD::CmdSendCSD, csd)) {
        goto failed;
    }

    mCapacity = getCapacity(csd);
    if (mCapacity == 0) {
        goto failed;
    }

    if (!readCxD(MMCSD::CmdSendCID, cid)) {
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


bool SDCardSpi::beginWrite(unsigned startblk)
{
    if (!blockAddresses) {
        startblk *= MMCSD::BlockSize;
    }

    if (sendCommand(MMCSD::CmdSetBlockCount, 1)           != MMCSD::R1Success ||
        sendCommand(MMCSD::CmdWriteMultiBlock, startblk)  != MMCSD::R1Success)
    {
        return false;
    }

    return true;
}

bool SDCardSpi::writeBlock(const uint8_t *buf, unsigned len)
{
    if (!waitForIdle(0xfff)) {
        return false;
    }

    const uint8_t start[] = { 0xff, MMCSD::DataWriteMultiple };
    dmaInProgress = true;
    spi.txDma(start, sizeof(start));
    waitForDma();

    dmaInProgress = true;
    spi.txDma(buf, MMCSD::BlockSize);
    waitForDma();

    uint8_t b[] = { 0xff, 0xff, 0xff };   // 2 bytes of CRC, 1 of status
    dmaInProgress = true;
    spi.transferDma(b, b, sizeof b);
    waitForDma();

    uint8_t status = b[2];
    if ((status & 0x1F) != MMCSD::DataAccepted) {
        // Handle error. deinit spi?
        spi.end();
        return false;
    }

    return true;
}

bool SDCardSpi::write(uint32_t startblk, const uint8_t *buf, unsigned len)
{
    /*
     * XXX: make fast path for single block writes
     */

    if (!beginWrite(startblk)) {
        return false;
    }

    if (!writeBlock(buf, len)) {
        return false;
    }

    static const uint8_t stop[] = { 0xFD, 0xFF };
    dmaInProgress = true;
    spi.txDma(stop, sizeof stop);
    waitForDma();

    spi.end();

    return true;
}

bool SDCardSpi::erase(uint32_t startblk, uint32_t endblk)
{
    if (!blockAddresses) {
        startblk *= MMCSD::BlockSize;
        endblk *= MMCSD::BlockSize;
    }

    uint8_t r1 = sendCommand(MMCSD::CmdEraseRWBlockStart, startblk);
    if (r1 != MMCSD::R1Success) {
        goto failed;
    }

    r1 = sendCommand(MMCSD::CmdEraseRWBlockEnd, endblk);
    if (r1 != MMCSD::R1Success) {
        goto failed;
    }

    r1 = sendCommand(MMCSD::CmdErase, 0);
    if (r1 != MMCSD::R1Success) {
        goto failed;
    }

    waitForResponse(0xff);

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

uint8_t SDCardSpi::waitForResponse(unsigned tries)
{
    /*
     * While busy, the SD Card will reply with 0xff.
     *
     * XXX: there appears to be no better way to detect the busy
     *      state of the card, but this should at least be interrupt
     *      driven on a per-byte basis.
     */

    uint8_t r = 0xff;
    while (tries-- && (r & 0x80)) {
        r = spi.transfer(0xff);
    }

    return r;
}

bool SDCardSpi::waitForIdle(unsigned tries)
{
    /*
     * While busy, the SD Card will reply with 0xff.
     *
     * XXX: there appears to be no better way to detect the busy
     *      state of the card, but this should at least be interrupt
     *      driven on a per-byte basis.
     */

    while (tries--) {
        if (spi.transfer(0xff) == 0xff)
            return true;
    }

    return false;
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
