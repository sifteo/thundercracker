/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "crc.h"

static uint32_t gCurrentWord;
static uint32_t gCrcTable[32];
static bool gCrcInit = false;


static uint32_t crcshift(uint32_t crctab[])
{
    uint32_t s = crctab[0];
    for (unsigned i = 1; i < 32; ++i)
        s += (crctab[i] << i);
    return s;
}

static uint32_t crcWord(uint32_t crctab[], const uint32_t word)
{
    static const uint32_t poly = 0x04C11DB7;
    for (unsigned j = 0; j < 32; ++j) {
        uint32_t crctmp = crcshift(crctab);
        crctab[0] = ((word >> (31 - j)) & 0x1 ) ^ crctab[31];
        for (unsigned i = 1; i < 32; ++i)
            crctab[i] = (crctab[0] & ((poly >> i) & 0x1)) ^ ((crctmp >> (i - 1)) & 0x1);
    }
    return crcshift(crctab);
}

void Crc32::init()
{
    gCrcInit = true;
}

void Crc32::deinit()
{
    gCrcInit = false;
}

void Crc32::reset()
{
    ASSERT(gCrcInit);
    for (unsigned i = 0; i < arraysize(gCrcTable); ++i) {
        gCrcTable[i] = 1;
    }
    gCurrentWord = 0;
}

uint32_t Crc32::get()
{
    ASSERT(gCrcInit);
    return gCurrentWord;
}

void Crc32::add(uint32_t word)
{
    ASSERT(gCrcInit);
    gCurrentWord = crcWord(gCrcTable, word);
}
