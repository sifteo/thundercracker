/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash.h"
#include "system.h"
#include "system_mc.h"
#include "flash_storage.h"


void Flash::read(uint32_t address, uint8_t *buf, unsigned len)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address <= sizeof storage.bytes &&
        len <= sizeof storage.bytes &&
        address + len <= sizeof storage.bytes) {

        memcpy(buf, storage.bytes + address, len);
    } else {
        ASSERT(0 && "MC flash read() out of range");
    }
}

void Flash::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address <= sizeof storage.bytes &&
        len <= sizeof storage.bytes &&
        address + len <= sizeof storage.bytes) {

        // Program bits from 1 to 0 only.
        while (len) {
            storage.bytes[address] &= *buf;
            buf++;
            address++;
            len--;
        }

    } else {
        ASSERT(0 && "MC flash write() out of range");
    }
}

void Flash::eraseSector(uint32_t address)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address < Flash::CAPACITY) {
        // Address can be anywhere inside the actual sector
        unsigned sector = address - (address % Flash::SECTOR_SIZE);
        memset(storage.bytes + sector, 0xFF, Flash::SECTOR_SIZE);
    } else {
        ASSERT(0 && "MC flash eraseSector() out of range");
    }
}

void Flash::chipErase()
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;
    memset(storage.bytes, 0xFF, sizeof storage.bytes);
}

void Flash::init()
{
    // No-op in the simulator
}

bool Flash::writeInProgress()
{
    // This is never async in the simulator
    return false;
}
