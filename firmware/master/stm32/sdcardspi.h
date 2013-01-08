#ifndef SD_CARD_H
#define SD_CARD_H

#include "spi.h"

class SDCardSpi
{
public:
    SDCardSpi(SPIMaster spimaster) :
        spi(spimaster)
    {}

    bool connect();

    bool read(uint32_t startblk, uint8_t *buf, unsigned numBlocks);
    bool write(uint32_t startblk, const uint8_t *buf, unsigned len);

private:
    static const unsigned CMD0_RETRY    = 10;
    static const unsigned CMD1_RETRY    = 100;
    static const unsigned ACMD41_RETRY  = 100;
    static const unsigned WAIT_DATA     = 10000;

    SPIMaster spi;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t capacity;

    void sendHeader(uint8_t cmd, uint32_t arg);

    uint8_t receiveR1();
    uint8_t receiveR3(uint8_t* buf);

    uint8_t sendCommandR1(uint8_t cmd, uint32_t arg);
    uint8_t sendCommandR3(uint8_t cmd, uint32_t arg, uint8_t *response);

    bool readCxD(uint8_t cmd, uint32_t cxd[4]);
    bool readBlock(uint8_t *buf, unsigned len);

    bool erase(uint32_t startblk, uint32_t endblk);

    static const uint8_t crc7LUT[256];
    static ALWAYS_INLINE uint8_t crc7(uint8_t crc, const uint8_t *buf, unsigned len) {
        while (len--) {
            crc = crc7LUT[(crc << 1) ^ (*buf++)];
        }

        return crc;
    }
    static uint32_t getCapacity(uint32_t csd[4]);
    static uint32_t getSlice(uint32_t *data, uint32_t end, uint32_t start);
};

#endif // SD_CARD_H
