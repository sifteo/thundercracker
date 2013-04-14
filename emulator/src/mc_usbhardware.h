#ifndef MC_USBHARDWARE_H
#define MC_USBHARDWARE_H

#include <stdint.h>
#include <vector>
#include <queue>

#ifdef _WIN32

#else
#   include <sys/select.h>
#endif

class UsbHardwareMC
{
public:
    static UsbHardwareMC& instance() {
        return _instance;
    }

    void init(const char *host = "127.0.0.1", const char *port = "2405");
    void work();
    bool isConnected() const {
        return connected;
    }

    uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);
    uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);

private:
    static const unsigned USB_HW_HDR_LEN = 1;

    UsbHardwareMC();
    static UsbHardwareMC _instance;

    struct addrinfo *addr;
    fd_set rfds, wfds, efds;
    int fd;

    bool connected;

    typedef std::vector<uint8_t> RxPacket;
    std::queue<RxPacket> bufferedOUTPackets;

    void tryConnect();
    void disconnect();

    void processRxData();
};

#endif // MC_USBHARDWARE_H
