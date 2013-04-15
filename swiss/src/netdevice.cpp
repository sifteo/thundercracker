#include "netdevice.h"
#include "macros.h"
#include "libusb.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#endif

#include <errno.h>

using namespace std;

NetDevice::NetDevice() :
    listenfd(-1),
    clientfd(-1)
{
}

bool NetDevice::open(uint16_t vendorId, uint16_t productId, uint8_t interface)
{
    // XXX: provide option to set port
    static const char *port = "2405";

    unsigned long arg;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    struct addrinfo hints;
    bzero(&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, port, &hints, &addr);
    if (err) {
        fprintf(stderr, "getaddrinfo err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    listenfd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (listenfd < 0) {
        fprintf(stderr, "socket err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    arg = 1;
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof arg);
    if (err) {
        fprintf(stderr, "setsockopt err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    err = bind(listenfd, addr->ai_addr, addr->ai_addrlen);
    if (err) {
        fprintf(stderr, "bind err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    err = listen(listenfd, 1);
    if (err) {
        fprintf(stderr, "listen err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    printf("swiss: waiting for client to connect on port %s...\n", port);

    socklen_t addrsize = sizeof clientaddr;
    clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrsize);
    if (clientfd < 0) {
        fprintf(stderr, "accept err: %s (%d)\n", strerror(errno), errno);
        return false;
    }

#ifdef _WIN32
    arg = 1;
    ioctlsocket(clientfd, FIONBIO, &arg);
#else
    err = fcntl(clientfd, F_SETFL, O_NONBLOCK | fcntl(clientfd, F_GETFL));
    if (err) {
        fprintf(stderr, "fcntl err: %s (%d)\n", strerror(errno), errno);
        return false;
    }
#endif

    printf("connected to client.\n");   // XXX: print client details

    return true;
}

void NetDevice::close()
{
    closefd(clientfd);
    closefd(listenfd);
}

void NetDevice::closefd(int &fd)
{
    if (fd >= 0) {
#ifdef _WIN32
        closesocket(fd);
#else

        int err = ::close(fd);
        if (err) {
            fprintf(stderr, "close err: %s (%d)\n", strerror(errno), errno);
        }
#endif
    }

    fd = -1;
}

bool NetDevice::isOpen() const
{
    return clientfd >= 0;
}

int NetDevice::processEvents(unsigned timeoutMillis)
{
    FD_SET(clientfd, &rfds);
    FD_SET(clientfd, &efds);

    if (false) { // txPending) {
        FD_SET(clientfd, &wfds);
    } else {
        FD_CLR(clientfd, &wfds);
    }

    struct timeval tv = {
        0,                      // tv_sec
        timeoutMillis * 1000    // tv_usec
    };

    int nfds = select(clientfd + 1, &rfds, &wfds, &efds, &tv);
    if (nfds <= 0) {
        return 0;
    }

    // Handle socket exceptions
    if (FD_ISSET(clientfd, &efds)) {
        close();
    }

    // enough room to receive more data?
    if (FD_ISSET(clientfd, &rfds)) {
        consume();
    }

#if 0
    // XXX: handle data written events?
    // data finished writing?
    if (FD_ISSET(clientfd, &wfds)) {
        // ?
    }
#endif

    return 0;
}

void NetDevice::consume()
{
    /*
     * Process received packets.
     * Complete packets are added to mBufferedINPackets.
     *
     * XXX: this does not currently handle incomplete packets.
     */

    uint8_t packetlen;
    int rxed = ::recv(clientfd, &packetlen, sizeof packetlen, 0);
    if (rxed <= 0) {
        close();
        return;
    }

    RxPacket pkt;
    pkt.resize(packetlen - 1);

    rxed = ::recv(clientfd, &pkt[0], pkt.size(), 0);
    if (rxed > 0) {
        mBufferedINPackets.push(pkt);
    } else if (errno != EAGAIN) {
        close();
    }
}

int NetDevice::readPacket(uint8_t *buf, unsigned maxlen, unsigned & rxlen)
{
    /*
     * Pop a previously received packet into the caller's buf.
     */

    assert(!mBufferedINPackets.empty());

    const RxPacket &pkt = mBufferedINPackets.front();
    rxlen = MIN(maxlen, pkt.size());
    memcpy(buf, &pkt[0], rxlen);

    mBufferedINPackets.pop();

    return LIBUSB_TRANSFER_COMPLETED;
}

int NetDevice::writePacket(const uint8_t *buf, unsigned len)
{
    /*
     * Write a packet directly to our socket.
     *
     * We copy the payload such that it's contiguous with our
     * small header so we can write it in a single send().
     */

    uint8_t pktbuf[MAX_EP_SIZE + 1];
    ASSERT(len <= MAX_EP_SIZE);

    struct MsgPacket {
        uint8_t len;
        uint8_t payload[1];
    } *packet = (MsgPacket *) pktbuf;

    packet->len = len;
    memcpy(packet->payload, buf, len);

    int sent = ::send(clientfd, pktbuf, packet->len + USB_HW_HDR_LEN, 0);
    if (sent < 0) {
        fprintf(stderr, "send1 err: %s (%d)\n", strerror(errno), errno);
    }

    return len;
}
