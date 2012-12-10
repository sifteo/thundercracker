#ifndef SDIOHOST_H
#define SDIOHOST_H

#include "hardware.h"

class SdioHost
{
public:
    enum BusMode {
        Mode1Bit    = (0 << 11),
        Mode4Bit    = (1 << 11),
        Mode8Bit    = (2 << 11),
        ModeMask    = (3 << 11)
    };

    struct Config {
        BusMode mode;
    };

    SdioHost(volatile SDIO_t *sdio) :
        hw(sdio)
    {}

    void init(const Config &cfg);

    bool connect();
    bool disconnect();
    bool sync();
    bool getDeviceInfo();

    bool read(uint32_t startblk, uint8_t *buf, uint32_t n);
    bool write(uint32_t startblk, const uint8_t *buf, uint32_t n);
    bool erase(uint32_t startblk, uint32_t endblk);

    void startClock();
    void setDataClock();
    void setBusMode(BusMode mode);
    void stopClock();

private:
    volatile SDIO_t *hw;
    volatile DMAChannel_t *dmaTx;

    static const uint32_t LOW_SPEED_DIV = 178;
    static const uint32_t HIGH_SPEED_DIV = 1;

    void sendCmdNone(uint8_t cmd, uint32_t arg);
    bool sendCmdShort(uint8_t cmd, uint32_t arg, uint32_t *resp);
    bool sendCmdShortCrc(uint8_t cmd, uint32_t arg, uint32_t *resp);
    bool sendCmdLongCrc(uint8_t cmd, uint32_t arg, uint32_t *resp);
};

#endif // SDIOHOST_H
