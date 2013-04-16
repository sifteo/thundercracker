#ifndef NETDEVICE_H
#define NETDEVICE_H

#include "iodevice.h"

#include <vector>
#include <queue>

#ifdef _WIN32

#else
#   include <sys/socket.h>
#   include <sys/select.h>
#endif

class NetDevice : public IODevice
{
public:
    NetDevice();

    bool open(uint16_t vendorId, uint16_t productId, uint8_t interface = 0);
    void close();
    bool isOpen() const;
    int  processEvents(unsigned timeoutMillis = 0);

    unsigned maxINPacketSize() const {
        return MAX_EP_SIZE;
    }
    unsigned numPendingINPackets() const {
        return mBufferedINPackets.size();
    }
    int readPacket(uint8_t *buf, unsigned maxlen, unsigned & rxlen);

    unsigned maxOUTPacketSize() const {
        return MAX_EP_SIZE;
    }
    unsigned numPendingOUTPackets() const {
        // we only track number of outstanding bytes,
        // so this will only be an approximation.
        return txPending / MAX_EP_SIZE;
    }
    int writePacket(const uint8_t *buf, unsigned len);

private:
    static const unsigned USB_HW_HDR_LEN = 1;   // 1 byte of length

    struct addrinfo *addr;
    struct sockaddr_storage clientaddr;

    int listenfd;
    int clientfd;
    fd_set rfds, wfds, efds;

    struct IOBuffer {
        uint16_t head;
        uint16_t tail;   // head <= tail
        uint8_t bytes[MAX_EP_SIZE * MAX_OUTSTANDING_OUT_TRANSFERS * 2];

        IOBuffer() :
            head(0), tail(0)
        {}
    };

    IOBuffer txbuf;
    IOBuffer rxbuf;
    unsigned txPending;

    void closefd(int &fd);
    void consume();
    void send();

    typedef std::vector<uint8_t> RxPacket;
    std::queue<RxPacket> mBufferedINPackets;
};

#endif // NETDEVICE_H
