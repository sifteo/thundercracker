#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

class FlashStorage
{
public:
    FlashStorage()
    {}

    void init();
    void read(uint32_t address, uint8_t *buf, unsigned len);
    bool write(uint32_t address, const uint8_t *buf, unsigned len);
    bool eraseSector(uint32_t address);
    bool chipErase();

private:
    static const char *filename;
};

#endif // FLASHSTORAGE_H
