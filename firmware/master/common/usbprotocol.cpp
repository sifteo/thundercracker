/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usbprotocol.h"
#include "flash_device.h"
#include "macros.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "hardware.h"
#include "factorytest.h"
#endif

/*
 * Table of USB subsystem handlers.
 * Order in this table must match the SubSystem enum.
 */
const USBProtocol::SubSystemHandler USBProtocol::subsystemHandlers[] = {
    USBProtocolHandler::installerHandler,   // 0
    #if ((defined SIFTEO_SIMULATOR) || (BOARD == BOARD_TEST_JIG))
    0
    #else
    FactoryTest::usbHandler,                // 1
    #endif
};

void USBProtocol::dispatch(const USBProtocolMsg &m)
{
    unsigned subsys = static_cast<unsigned>(m.subsystem());
    if (subsys < arraysize(subsystemHandlers)) {
        SubSystemHandler handler = subsystemHandlers[subsys];
        handler(m.bytes, m.len);
    }
}



struct USBProtocolHandler::AssetInstallation USBProtocolHandler::installation;

void USBProtocolHandler::init()
{
    installation.state = WaitingForLength;
}

// XXX: static so it hangs around until the usb device actually writes it out
static uint8_t status;

/*
    Totally fake for now.
    Just assume all data coming in here is to be written to flash, the first 4
    bytes we receive are the length.
    But, calculate the CRC as it goes in & verify it on the data written to flash.
*/
void USBProtocolHandler::installerHandler(const uint8_t *buf, unsigned len)
{
    // first word is the header
    buf += USBProtocolMsg::HEADER_BYTES;
    len -= USBProtocolMsg::HEADER_BYTES;

    if (installation.state == WaitingForLength) {
        // XXX: chokes if we don't get the 4 bytes of length at once :/
        installation.size = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
        buf += sizeof(installation.size);
        len -= sizeof(installation.size);
        installation.currentAddress = 0;    // XXX: only writing to beginning of flash for now
        installation.crcwordBytes = 0;
        installation.crcword = 0;

        // make sure enough sectors are erased for this asset
        // XXX: just starting from 0 for now, assuming only one game's assets
        for (unsigned i = 0; i < installation.size; i += FlashDevice::SECTOR_SIZE) {
#ifndef SIFTEO_SIMULATOR
            status = 2;
            UsbDevice::write(&status, 1);
#endif
            FlashDevice::eraseSector(i);
        }
#ifndef SIFTEO_SIMULATOR
        status = 0x0;
        UsbDevice::write(&status, 1);

        CRC.CR = 1; // reset CRC unit
#endif
        installation.state = WritingData;
    }

    unsigned chunk = MIN(installation.size - installation.currentAddress, len);

    FlashDevice::write(installation.currentAddress, buf, chunk);
    installation.currentAddress += chunk;

    addToCrc(buf, chunk);

    // are we done?
    // XXX: assumes 0 based address, boo
    if (installation.currentAddress >= installation.size) {
        installation.state = WaitingForLength;

        uint32_t crc;
#ifndef SIFTEO_SIMULATOR
        crc = CRC.DR;

        CRC.CR = 1; // reset CRC for verification
#endif

        installation.crcwordBytes = 0;
        installation.crcword = 0;

        // wait for the last transaction to finish
        while (FlashDevice::writeInProgress())
            ;
        // debug: read back out and verify CRC
        uint8_t b[FlashDevice::PAGE_SIZE];
        unsigned addr = 0;
        while (addr < installation.size) {  // XXX: assumes 0 based offset
            unsigned chunksize = MIN(installation.size - addr, sizeof(b));
            FlashDevice::read(addr, b, chunksize);
            addToCrc(b, chunksize);
            addr += chunksize;
        }
#ifndef SIFTEO_SIMULATOR
        status = (crc == CRC.DR) ? 0 : 1;
        UsbDevice::write(&status, 1);
#endif
    }

}

/*
    Gather 4-byte words of data and update the CRC.
    Track the number of bytes buffered in crcword.
*/
void USBProtocolHandler::addToCrc(const uint8_t *buf, unsigned len)
{
    while (len) {
        installation.crcword |= (*buf << (installation.crcwordBytes * 8));
        installation.crcwordBytes++;
        buf++;
        len--;

        if (installation.crcwordBytes == 4) {
#ifndef SIFTEO_SIMULATOR
            CRC.DR = installation.crcword;
#endif
            installation.crcwordBytes = 0;
            installation.crcword = 0;
        }
    }
}

