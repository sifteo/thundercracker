/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "system.h"
#include "system_mc.h"
#include "mc_timing.h"
#include "svmmemory.h"
#include "flash_device.h"
#include "flash_storage.h"
#include "lua_filesystem.h"

static int gStealthIOCounter;


void FlashDevice::setStealthIO(int counter)
{
    /*
     * It's helpful to have a way for internal simulator components to
     * issue "stealth" I/O, which doesn't show up for example in our Lua
     * hooks. This is used when reading debug symbols.
     *
     * The public interface for this feature is a global counter which
     * can be incremented before doing a stealth operation, and decremented
     * afterward.
     */

    gStealthIOCounter += counter;
    ASSERT(gStealthIOCounter >= 0);
    ASSERT(gStealthIOCounter <= 4);
}

void FlashDevice::read(uint32_t address, uint8_t *buf, unsigned len)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address <= sizeof storage.bytes &&
        len <= sizeof storage.bytes &&
        address + len <= sizeof storage.bytes) {

        memcpy(buf, storage.bytes + address, len);
    } else {
        ASSERT(0 && "MC flash read() out of range");
    }

    if (!gStealthIOCounter) {
        LuaFilesystem::onRawRead(address, buf, len);
        SystemMC::elapseTicks(MCTiming::TICKS_PER_PAGE_MISS);
    }
}

void FlashDevice::verify(uint32_t address, const uint8_t *buf, unsigned len)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    ASSERT(address <= sizeof storage.bytes &&
           len <= sizeof storage.bytes &&
           address + len <= sizeof storage.bytes);

    ASSERT(0 == memcmp(buf, storage.bytes + address, len));
}

void FlashDevice::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address <= sizeof storage.bytes &&
        len <= sizeof storage.bytes &&
        address + len <= sizeof storage.bytes) {

        if (!gStealthIOCounter) {
            LuaFilesystem::onRawWrite(address, buf, len);
            SystemMC::elapseTicks(MCTiming::TICKS_PER_PAGE_WRITE);
        }

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

void FlashDevice::eraseBlock(uint32_t address)
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    if (address < FlashDevice::MAX_CAPACITY) {
        // Address can be anywhere inside the actual sector
        unsigned sector = address - (address % FlashDevice::ERASE_BLOCK_SIZE);

        if (!gStealthIOCounter) {
            // Log non-stealth erases, since these will introduce a visible performance hiccup.
            LOG(("FLASH: Erasing block %08x\n", address));

            LuaFilesystem::onRawErase(address);
            SystemMC::elapseTicks(MCTiming::TICKS_PER_BLOCK_ERASE);
        }

        memset(storage.bytes + sector, 0xFF, FlashDevice::ERASE_BLOCK_SIZE);
        storage.eraseCounts[sector / FlashDevice::ERASE_BLOCK_SIZE]++;

    } else {
        ASSERT(0 && "MC flash eraseSector() out of range");
    }
}

void FlashDevice::eraseAll()
{
    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;
    memset(storage.bytes, 0xff, sizeof storage.bytes);
    for (unsigned i = 0; i < arraysize(storage.eraseCounts); ++i)
        storage.eraseCounts[i]++;
}

bool FlashDevice::busy()
{
    // everything is done synchronously in simulation
    return false;
}

void FlashDevice::init()
{
    // No-op in the simulator
}
