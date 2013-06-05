/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * The lowest (first) layer of the flash stack: Physical access to the
 * flash device. The implementation of this layer is platform-specific.
 */

#ifndef FLASH_DEVICE_H
#define FLASH_DEVICE_H

#include <stdint.h>
#include "macros.h"

class FlashDevice {
public:
    static const unsigned PAGE_SIZE = 256;                  // programming granularity
    static const unsigned ERASE_BLOCK_SIZE = 1024 * 64;     // coarse erase granularity
    static const unsigned MAX_CAPACITY = 1024 * 1024 * 32;  // Max capacity currently supported

    static const unsigned capacity();
    static const uint8_t mfgr_id();

    static void init();

    static void read(uint32_t address, uint8_t *buf, unsigned len);
    static void write(uint32_t address, const uint8_t *buf, unsigned len);

    DEBUG_ONLY(static void setStealthIO(int counter);)
    DEBUG_ONLY(static void verify(uint32_t address, const uint8_t *buf, unsigned len);)

    static void eraseBlock(uint32_t address);
    static void eraseAll();

    static bool busy();

    struct JedecID {
        uint8_t manufacturerID;
        uint8_t memoryType;
        uint8_t memoryDensity;
    };

    static void readId(JedecID *id);
};


/**
 * A utility class that enables "stealth" flash I/O in simulation
 * as long as it's in scope. No effect on hardware.
 */

class FlashScopedStealthIO {
public:
    FlashScopedStealthIO() {
        DEBUG_ONLY(FlashDevice::setStealthIO(1);)
    }

    ~FlashScopedStealthIO() {
        DEBUG_ONLY(FlashDevice::setStealthIO(-1);)
    }
};


#endif

