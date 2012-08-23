/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macronixmx25.h"
#include "flash_device.h"
#include "board.h"
#include "macros.h"
#include "tasks.h"
#include "sampleprofiler.h"
#include "systime.h"

volatile bool MacronixMX25::dmaInProgress = false;

void MacronixMX25::init()
{
    GPIOPin writeProtect = FLASH_WP_GPIO;
    writeProtect.setControl(GPIOPin::OUT_2MHZ);
    writeProtect.setHigh();

    spi.init();

    // prepare to write the status register
    spi.begin();
    spi.transfer(WriteEnable);
    spi.end();

    spi.begin();
    spi.transfer(WriteStatusConfigReg);
    spi.transfer(0);    // ensure block protection is disabled
    spi.end();

    mightBeBusy = false;
    while (readReg(ReadStatusReg) != Ok) {
        ; // sanity checking?
    }
}

void MacronixMX25::read(uint32_t address, uint8_t *buf, unsigned len)
{
    const uint8_t cmd[] = { FastRead,
                            address >> 16,
                            address >> 8,
                            address >> 0,
                            Nop };  // dummy

    waitWhileBusy();

    // May need to retry in case of DMA failure
    while (1) {

        spi.begin();

        dmaInProgress = true;
        spi.txDma(cmd, sizeof(cmd));
        if (!waitForDma()) {
            spi.end();
            continue;
        }

        dmaInProgress = true;
        spi.transferDma(buf, buf, len);
        if (!waitForDma()) {
            spi.end();
            continue;
        }

        spi.end();
        return;
    }
}

/*
    Simple synchronous writing.
*/
void MacronixMX25::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    while (len) {
        // align writes to PAGE_SIZE chunks
        uint32_t pagelen = FlashDevice::PAGE_SIZE - (address & (FlashDevice::PAGE_SIZE - 1));
        if (pagelen > len) pagelen = len;

        waitWhileBusy();
        ensureWriteEnabled();

        const uint8_t cmd[] = { PageProgram,
                                address >> 16,
                                address >> 8,
                                address >> 0 };

        // May need to retry in case of DMA failure
        while (1) {

            spi.begin();
            dmaInProgress = true;
            spi.txDma(cmd, sizeof(cmd));
            if (!waitForDma()) {
                spi.end();
                continue;
            }

            dmaInProgress = true;
            spi.txDma(buf, pagelen);
            if (!waitForDma()) {
                spi.end();
                continue;
            }

            spi.end();
            break;
        }

        buf += pagelen;
        len -= pagelen;
        address += pagelen;

        mightBeBusy = true;
    }
}

/*
 * Any address within a block is valid to erase that block.
 */
void MacronixMX25::eraseBlock(uint32_t address)
{
    waitWhileBusy();
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(BlockErase64);
    spi.transfer(address >> 16);
    spi.transfer(address >> 8);
    spi.transfer(address >> 0);
    spi.end();

    mightBeBusy = true;
}

void MacronixMX25::chipErase()
{
    waitWhileBusy();
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(ChipErase);
    spi.end();

    mightBeBusy = true;
}

void MacronixMX25::ensureWriteEnabled()
{
    do {
        spi.begin();
        spi.transfer(WriteEnable);
        spi.end();
    } while (!(readReg(ReadStatusReg) & WriteEnableLatch));
}

void MacronixMX25::readId(FlashDevice::JedecID *id)
{
    waitWhileBusy();
    spi.begin();

    spi.transfer(ReadID);
    id->manufacturerID = spi.transfer(Nop);
    id->memoryType = spi.transfer(Nop);
    id->memoryDensity = spi.transfer(Nop);

    spi.end();
}

/*
 * Valid for any register that returns one byte of data in response
 * to a command byte. Only used internally for reading status and security
 * registers at the moment.
 */
uint8_t MacronixMX25::readReg(MacronixMX25::Command cmd)
{
    uint8_t reg;
    spi.begin();
    spi.transfer(cmd);
    reg = spi.transfer(Nop);
    spi.end();
    return reg;
}

void MacronixMX25::deepSleep()
{
    spi.begin();
    spi.transfer(DeepPowerDown);
    spi.end();
}

void MacronixMX25::wakeFromDeepSleep()
{
    spi.begin();
    spi.transfer(ReleaseDeepPowerDown);
    // 3 dummy bytes
    spi.transfer(Nop);
    spi.transfer(Nop);
    spi.transfer(Nop);
    // 1 byte old style electronic signature - could return this if we want to...
    spi.transfer(Nop);
    spi.end();
    // TODO - CSN must remain high for 30us on transition out of deep sleep
}

void MacronixMX25::waitWhileBusy()
{
    if (mightBeBusy) {
        while (readReg(ReadStatusReg) & WriteInProgress) {
            // Kill time.. not safe to execute tasks here.
        }
        mightBeBusy = false;
    }
}

bool MacronixMX25::waitForDma()
{
    /*
     * Wait for an SPI DMA operation to complete.
     *
     * BIG CAVEAT: This very rarely can fail (time out) while waiting.
     *
     * This seems to be a hardware bug, possibly some kind of contention
     * between DMA engines. We've identified some workarounds (discussed
     * in SPIMaster::txDma) which reduce the likelyhood of the problem,
     * but it still seems highly unpredictable. Here's a blog post which
     * points to similar DMA failures in other STM32 models:
     *
     * http://blog.frankvh.com/2012/01/13/stm32f2xx-stm32f4xx-dma-maximum-transactions/
     *
     * Until we get to the bottom of this, we'll just be keeping the DMA
     * controller on a very short leash. If it takes longer than we
     * expect to complete the DMA, we'll assume it's lost and start over.
     *
     * More notes:
     *
     *   - The error flags are NOT set on either the TX or RX DMA channel
     *
     *   - When I've dumped the register contents after timeout, the TX
     *     DMA has completed, but the RX DMA is hung with one byte left.
     *
     *   - If I simply restart the SPI transaction from the beginning,
     *     the next one will hang immediately too. The SPI controller
     *     must be reset in order to recover. This seems similar to the
     *     "buggy peripherals" hypothesis from the blog post above.
     *
     *   - Placing more code in this loop seems to make the hang less
     *     likely, adding fuel to my suspicion that this is a memory
     *     controller bug in part.
     */
    
    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::FlashDMA);

    SysTime::Ticks deadline = SysTime::ticks() + SysTime::msTicks(5);
    bool success = true;

    while (dmaInProgress && success) {
        /*
         * Kill time.. not safe to execute tasks here. We can yield until
         * the DMA IRQ comes back, to give the DMA controller more bus bandwidth.
         */
        Tasks::waitForInterrupt();

        if (SysTime::ticks() > deadline) {
            /*
             * Recover from terrible hardware badness!
             */
            UART("DMA timeout\r\n");
            spi.init();
            success = false;
        }
    }

    SampleProfiler::setSubsystem(s);
    return success;
}

void MacronixMX25::dmaCompletionCallback()
{
    dmaInProgress = false;
}
