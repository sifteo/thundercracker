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
    void write(uint32_t address, const uint8_t *buf, unsigned len);
    void eraseSector(uint32_t address);
    void chipErase();
    void flush();

private:
    FILE *file;
    uint8_t *data;
    static const char *filename;
};

#endif // FLASHSTORAGE_H
