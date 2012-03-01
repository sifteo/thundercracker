/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include <stdio.h>

class FlashStorage
{
public:
    FlashStorage()
    {}

    bool init();
    void read(uint32_t address, uint8_t *buf, unsigned len);
    bool write(uint32_t address, const uint8_t *buf, unsigned len);
    bool eraseSector(uint32_t address);
    bool chipErase();
    bool flush();

private:
    FILE *file;
    uint8_t *data;
    static const char *filename;
};

#endif // FLASHSTORAGE_H
