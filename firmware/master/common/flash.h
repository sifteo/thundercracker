/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

class Flash {
public:
    static const unsigned PAGE_SIZE = 256;              // programming granularity
    static const unsigned SECTOR_SIZE = 1024 * 4;       // smallest erase granularity
    static const unsigned CAPACITY = 1024 * 1024 * 16;  // total storage capacity

    static void init();

    static void read(uint32_t address, uint8_t *buf, unsigned len);
    static void write(uint32_t address, const uint8_t *buf, unsigned len);
    static void eraseSector(uint32_t address);
    static void chipErase();
    static bool writeInProgress();

    // sim only
    static void flush();
};

#endif // FLASH_H
