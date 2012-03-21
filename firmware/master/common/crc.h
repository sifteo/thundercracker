/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

class Crc32 {
public:
    static void init();
    static void deinit();

    static void reset();
    static uint32_t get();
    static void add(uint32_t word);

#ifdef SIFTEO_SIMULATOR
private:
    static uint32_t crcshift(uint32_t crctab[]);
    static uint32_t crcWord(uint32_t crctab[], const uint32_t word);

    static uint32_t currentWord;
    static uint32_t crctable[32];
#endif
};

#endif // CRC_H
