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
    static bool write(uint32_t address, const uint8_t *buf, unsigned len);
    static bool eraseSector(uint32_t address);
    static bool chipErase();

    // sim only
    static bool flush();
};

#endif // FLASH_H
