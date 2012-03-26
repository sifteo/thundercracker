/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macronixmx25.h"
#include "flash.h"
#include "board.h"
#include "macros.h"

void MacronixMX25::init()
{
    GPIOPin writeProtect = FLASH_WP_GPIO;
    writeProtect.setControl(GPIOPin::OUT_2MHZ);
    writeProtect.setHigh();

#if (BOARD == BOARD_TC_MASTER_REV2)
    GPIOPin regEnable = FLASH_REG_EN_GPIO;
    regEnable.setControl(GPIOPin::OUT_2MHZ);
    regEnable.setHigh();
#endif

    spi.init();

    // reset
#if 0
    spi.begin();
    spi.transfer(ResetEnable);
    spi.end();

    spi.begin();
    spi.transfer(Reset);
    spi.end();
#endif

    // prepare to write the status register
    spi.begin();
    spi.transfer(WriteEnable);
    spi.end();

    spi.begin();
    spi.transfer(WriteStatusConfigReg);
    spi.transfer(0);    // ensure block protection is disabled
    spi.end();

    while (readReg(ReadStatusReg) != Ok) {
        ; // sanity checking?
    }

#if 0
    JedecID id;
    spi.begin();
    spi.transfer(ReadID);
    id.manufacturerID = spi.transfer(Nop);
    id.memoryType = spi.transfer(Nop);
    id.memoryDensity = spi.transfer(Nop);
    spi.end();
#endif
}

/*
    Simple synchronous reading.
    TODO - DMA!
*/
void MacronixMX25::read(uint32_t address, uint8_t *buf, unsigned len)
{
    const uint8_t cmd[] = { FastRead,
                            address >> 16,
                            address >> 8,
                            address >> 0,
                            Nop };  // dummy
    spi.begin();

    spi.txDma(cmd, sizeof(cmd));
    while (spi.dmaInProgress()) {
        ;
    }

    spi.transferDma(buf, buf, len);
    while (spi.dmaInProgress()) {
        ;
    }

    spi.end();
}

/*
    Simple synchronous writing.
*/
void MacronixMX25::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    while (len) {
        // align writes to PAGE_SIZE chunks
        uint32_t pagelen = Flash::PAGE_SIZE - (address & (Flash::PAGE_SIZE - 1));
        if (pagelen > len) pagelen = len;

        // wait for any previous write/erase to complete
        while (writeInProgress())
            ;
        ensureWriteEnabled();

        const uint8_t cmd[] = { PageProgram,
                                address >> 16,
                                address >> 8,
                                address >> 0 };
        spi.begin();
        spi.txDma(cmd, sizeof(cmd));

        len -= pagelen;
        address += pagelen;

        while (spi.dmaInProgress()) {
            ;
        }

        spi.txDma(buf, pagelen);
        buf += pagelen;
        while (spi.dmaInProgress()) {
            ;
        }
        spi.end();
    }
}

/*
 * Any address within a sector is valid to erase that sector.
 */
void MacronixMX25::eraseSector(uint32_t address)
{
    // wait for any previous write/erase to complete
    while (writeInProgress())
        ;
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(SectorErase);
    spi.transfer(address >> 16);
    spi.transfer(address >> 8);
    spi.transfer(address >> 0);
    spi.end();
}

/*
 * Any address within a block is valid to erase that block.
 */
void MacronixMX25::eraseBlock(uint32_t address)
{
    // wait for any previous write/erase to complete
    while (writeInProgress())
        ;
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(BlockErase64);
    spi.transfer(address >> 16);
    spi.transfer(address >> 8);
    spi.transfer(address >> 0);
    spi.end();
}

void MacronixMX25::chipErase()
{
    // wait for any previous write/erase to complete
    while (writeInProgress())
        ;
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(ChipErase);
    spi.end();
}

void MacronixMX25::ensureWriteEnabled()
{
    do {
        spi.begin();
        spi.transfer(WriteEnable);
        spi.end();
    } while (!(readReg(ReadStatusReg) & WriteEnableLatch));
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
