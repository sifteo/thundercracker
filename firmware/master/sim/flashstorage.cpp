/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash.h"
#include "flashstorage.h"
#include <sifteo.h>

#include <stdlib.h>
#include <string.h>

const char *FlashStorage::filename = "flashstorage.bin";

bool FlashStorage::init()
{
    // TODO: might be nicer to mmap this, especially if it gets any larger,
    // but no easy cross platform way
    data = static_cast<uint8_t*>(malloc(Flash::CAPACITY));
    ASSERT(data != NULL && "malloc fail");

    file = fopen(filename, "r+b");
    if (file == NULL) {
        file = fopen(filename, "w+b");
    }
    ASSERT(file != NULL && "couldn't access backing file for FlashStorage");

    memset(data, 0xFF, Flash::CAPACITY);  // in case file is somehow smaller than we want
    size_t rxed = fread(data, 1, Flash::CAPACITY, file);
    if (rxed < 0) {
        return false;
    }
    return true;
}

void FlashStorage::read(uint32_t address, uint8_t *buf, unsigned len)
{
    ASSERT((address + len < Flash::CAPACITY) && "invalid address");
    memcpy(buf, data + address, len);
}

void FlashStorage::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    ASSERT((address + len < Flash::CAPACITY) && "writing off the end of flash");
    memcpy(data + address, buf, len);
}

void FlashStorage::eraseSector(uint32_t address)
{
    ASSERT(address < Flash::CAPACITY && "invalid address");
    // address can be anywhere inside the actual sector
    unsigned sector = address - (address % Flash::SECTOR_SIZE);
    memset(data + sector, 0xFF, Flash::SECTOR_SIZE);
}

void FlashStorage::chipErase()
{
    ASSERT(file != NULL);
    memset(data, 0xFF, Flash::CAPACITY);
}

void FlashStorage::flush()
{
    fseek(file, 0, SEEK_SET);
    size_t result = fwrite(data, Flash::CAPACITY, 1, file);
    if (result != 1) {
        LOG(("Error writing flash\n"));
    }
    fflush(file);
}
