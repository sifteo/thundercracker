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

    static const uint8_t MACRONIX_MFGR_ID = 0xC2;

    static void init();

    static void read(uint32_t address, uint8_t *buf, unsigned len);
    static void write(uint32_t address, const uint8_t *buf, unsigned len);
    static bool writeInProgress();

    static void eraseSector(uint32_t address);
    static void chipErase();

    struct JedecID {
        uint8_t manufacturerID;
        uint8_t memoryType;
        uint8_t memoryDensity;
    };

    static void readId(JedecID *id);
};

#endif // FLASH_H
