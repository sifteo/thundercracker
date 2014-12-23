/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
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

    if (address < FlashDevice::CAPACITY) {
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
