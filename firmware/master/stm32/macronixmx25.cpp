#include "macronixmx25.h"
#include "board.h"

MacronixMX25 MacronixMX25::instance(SPIMaster(&FLASH_SPI,
                                              FLASH_CS_GPIO,
                                              FLASH_SCK_GPIO,
                                              FLASH_MISO_GPIO,
                                              FLASH_MOSI_GPIO));

void MacronixMX25::init()
{
    GPIOPin writeProtect = FLASH_WP_GPIO;
    writeProtect.setControl(GPIOPin::OUT_10MHZ);
    writeProtect.setHigh();

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
    spi.begin();
    spi.transfer(FastRead);
    spi.transfer(address >> 16);
    spi.transfer(address >> 8);
    spi.transfer(address >> 0);
    spi.transfer(Nop);  // dummy

    while (len--) {
        *buf++ = spi.transfer(Nop);
    }
    spi.end();
}

/*
    Simple synchronous writing.
    TODO - DMA!
*/
MacronixMX25::Status MacronixMX25::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    uint32_t pagelen;

    while (len) {
        // align writes to PAGE_SIZE chunks
        pagelen = PAGE_SIZE - (address & (PAGE_SIZE - 1));
        if (pagelen > len) pagelen = len;

        ensureWriteEnabled();

        spi.begin();
        spi.transfer(PageProgram);
        spi.transfer(address >> 16);
        spi.transfer(address >> 8);
        spi.transfer(address >> 0);

        len -= pagelen;
        address += pagelen;

        while (pagelen--) {
            spi.transfer(*buf++);
        }
        spi.end();

        // wait for the write to complete
        while (readReg(ReadStatusReg) & WriteInProgress) {
            ; // yuck - anything better to do here?
        }

        // security register provides failure bits for read & erase
        Status s = (Status)readReg(ReadSecurityReg);
        if (s != Ok) {
            return s;
        }
    }
    return Ok;
}

/*
 * Any address within a sector is valid to erase that sector.
 */
MacronixMX25::Status MacronixMX25::eraseSector(uint32_t address)
{
    ensureWriteEnabled();

    spi.begin();
    spi.transfer(SectorErase);
    spi.transfer(address >> 16);
    spi.transfer(address >> 8);
    spi.transfer(address >> 0);
    spi.end();

    // wait for erase complete
    while (readReg(ReadStatusReg) & WriteInProgress) {
        ; // do something better here :/
    }

    return (Status)(readReg(ReadSecurityReg) & (EraseFail | WriteProtected));
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
