
#include "crc.h"
#include <string.h>
#include <sifteo.h>

// statics
uint32_t Crc32::currentWord;
uint32_t Crc32::crctable[32];

uint32_t Crc32::crcshift(uint32_t crctab[])
{
    uint32_t s = crctab[0];
    for (unsigned i = 1; i < 32; ++i)
        s += (crctab[i] << i);
    return s;
}

uint32_t Crc32::crcWord(uint32_t crctab[], const uint32_t word)
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
    // nop in simulator
}

void Crc32::deinit()
{
    // nop in simulator
}

void Crc32::reset()
{
    for (unsigned i = 0; i < arraysize(crctable); ++i) {
        crctable[i] = 1;
    }
    currentWord = 0;
}

uint32_t Crc32::get()
{
    return currentWord;
}

void Crc32::add(uint32_t word)
{
    currentWord = crcWord(crctable, word);
}
