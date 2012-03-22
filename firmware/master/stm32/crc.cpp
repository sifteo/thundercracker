/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "crc.h"
#include "hardware.h"

void Crc32::init()
{
    RCC.AHBENR |= (1 << 6);
}

void Crc32::deinit()
{
    RCC.AHBENR &= ~(1 << 6);
}

void Crc32::reset()
{
    CRC.CR = (1 << 0);
}

uint32_t Crc32::get()
{
    return CRC.DR;
}

void Crc32::add(uint32_t word)
{
    CRC.DR = word;
}
