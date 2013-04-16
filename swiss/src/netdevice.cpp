#include "netdevice.h"
#include "macros.h"
#include "libusb.h"
#include "options.h"

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
    clientfd(-1),
    txPending(0)
{
}

bool NetDevice::open(uint16_t vendorId, uint16_t productId, uint8_t interface)
{
    const char *port = Options::netDevicePort();

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
    if (!isOpen()) {
        return -1;
    }

    FD_SET(clientfd, &rfds);
    FD_SET(clientfd, &efds);
    FD_SET(clientfd, &wfds);

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
    if (isOpen() && FD_ISSET(clientfd, &rfds)) {
        consume();
    }

    if (isOpen() && (txbuf.tail > txbuf.head) && FD_ISSET(clientfd, &wfds)) {
        send();
    }

    return 0;
}

void NetDevice::consume()
{
    // Pull new data out of the socket into our buffer.
    int rxed = ::recv(clientfd, rxbuf.bytes + rxbuf.tail, sizeof rxbuf.bytes - rxbuf.tail, 0);
    if (rxed > 0) {
        rxbuf.tail += rxed;
    } else if (errno != EAGAIN) {
        // if rxed == 0, assume client has disconnected
        if (rxed < 0) {
            perror("recv");
        }
        close();
        return;
    }

    /*
     * Process received data.
     * Complete packets are added to mBufferedINPackets.
     */

    for (;;) {
        unsigned bufferLen = rxbuf.tail - rxbuf.head;
        if (bufferLen < USB_HW_HDR_LEN) {
            break;
        }

        uint8_t *packet = &rxbuf.bytes[rxbuf.head];
        unsigned packetLen = packet[0] + USB_HW_HDR_LEN;
        if (packetLen > bufferLen) {
            break;
        }

        RxPacket pkt(packetLen - USB_HW_HDR_LEN, 0);
        memcpy(&pkt[0], &packet[1], pkt.size());
        mBufferedINPackets.push(pkt);

        rxbuf.head += packetLen;
    }

    /*
     * Reclaim buffer space. Typically we'll be receiving whole
     * packets, so this isn't expected to be a frequent operation.
     */

    if (rxbuf.head == rxbuf.tail) {
        rxbuf.head = rxbuf.tail = 0;
    } else if (rxbuf.head > sizeof rxbuf.bytes / 2) {
        rxbuf.tail -= rxbuf.head;
        memmove(rxbuf.bytes, rxbuf.bytes + rxbuf.head, rxbuf.tail);
        rxbuf.head = 0;
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
     * Buffer a packet to be written to our socket.
     */

    struct MsgPacket {
        uint8_t len;
        uint8_t payload[1];
    } *packet = (MsgPacket *) &txbuf.bytes[txbuf.tail];

    packet->len = len;
    memcpy(packet->payload, buf, len);
    txbuf.tail += len + USB_HW_HDR_LEN;
    txPending += len + USB_HW_HDR_LEN;

    return len;
}

void NetDevice::send()
{
    int written = ::send(clientfd, txbuf.bytes + txbuf.head, txbuf.tail - txbuf.head, 0);
    if (written > 0) {
        txbuf.head += written;
        txPending -= written;
    } else if (errno != EAGAIN) {
        perror("send");
        close();
    }

    if (txbuf.head == txbuf.tail) {
        txbuf.head = txbuf.tail = 0;
    }
}
