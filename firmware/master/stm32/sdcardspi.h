#ifndef SD_CARD_H
#define SD_CARD_H

#include "spi.h"
#include "mmcsd.h"

class SDCardSpi
{
public:
    SDCardSpi(SPIMaster spimaster) :
        spi(spimaster),
        blockAddresses(false)
    {}

    bool connect();

    unsigned capacity() const {
        return mCapacity;
    }

    bool read(uint32_t startblk, uint8_t *buf, unsigned numBlocks);
    bool write(uint32_t startblk, const uint8_t *buf, unsigned len);
    bool erase(uint32_t startblk, uint32_t endblk);

    static void dmaCompletionCallback();

private:
    static const unsigned CMD0_RETRY    = 10;
    static const unsigned CMD1_RETRY    = 100;
    static const unsigned ACMD41_RETRY  = 100;
    static const unsigned WAIT_DATA     = 10000;

    SPIMaster spi;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t mCapacity;
    bool blockAddresses;
    static volatile bool dmaInProgress;

    bool select();

    uint8_t sendCommand(uint8_t cmd, uint32_t arg);
    uint8_t sendOneCommand(uint8_t cmd, uint32_t arg);
    void sendHeader(uint8_t cmd, uint32_t arg);

    uint8_t receiveR1();
    uint8_t receiveR3(MMCSD::R7 &response);

    bool readCxD(uint8_t cmd, uint32_t cxd[4]);
    bool readBlock(uint8_t *buf, unsigned len);

    bool beginWrite(unsigned startblk);
    bool writeBlock(const uint8_t *buf, unsigned len);

    bool waitForDma();
    uint8_t waitForResponse(unsigned tries);
    bool waitForIdle(unsigned tries);
    void delayMillis(unsigned ms);

    static uint32_t getCapacity(uint32_t csd[4]);
    static uint32_t getSlice(uint32_t *data, uint32_t end, uint32_t start);
};

#endif // SD_CARD_H
