/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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
    static const unsigned CAPACITY = 1024 * 1024 * 16;      // total storage capacity

    static const uint8_t MACRONIX_MFGR_ID = 0xC2;
    static const uint8_t WINBOND_MFGR_ID = 0xEF;

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

