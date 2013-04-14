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
        return 0; // we don't buffer outgoing traffic
    }
    int writePacket(const uint8_t *buf, unsigned len);

private:
    struct addrinfo *addr;
    struct sockaddr_storage clientaddr;

    int listenfd;
    int clientfd;
    fd_set rfds, wfds, efds;

    void closefd(int &fd);
    void consume();

    typedef std::vector<uint8_t> RxPacket;
    std::queue<RxPacket> mBufferedINPackets;
};

#endif // NETDEVICE_H
