#include "mc_usbhardware.h"

#include "tasks.h"

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

#include <stdio.h>
#include <strings.h>
#include <errno.h>

using namespace std;

/*
 * Rough emulation the base's USB connection via a TCP socket.
 *
 * We make no real attempts to emulate accurate timing or throughput,
 * but mainly focus on having a pipe for data that serves the same purpose
 * as the USB connection.
 */

UsbHardwareMC UsbHardwareMC::_instance;

UsbHardwareMC::UsbHardwareMC() :
    fd(-1),
    connected(false)
{
}

void UsbHardwareMC::init(const char *host, const char *port)
{
    /*
     * Initialize the radio device, and try to connect. We may not be
     * able to connect right away, though, or we may get
     * dconnected. We'll try to reconnect as necessary.
     */

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    struct addrinfo hints;
    bzero(&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(host, port, &hints, &addr);
    if (err) {
        fprintf(stderr, "getaddrinfo: %s (%d)", gai_strerror(err), err);
    }
}

void UsbHardwareMC::tryConnect()
{
    if (fd < 0) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);

        fd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0) {
            return;
        }
    }

    if (::connect(fd, addr->ai_addr, addr->ai_addrlen)) {
        if (errno != ECONNREFUSED) {
            printf("connect: %s (%d)\n", strerror(errno), errno);
        }
        disconnect();
        return;
    }

    printf("INFO: usb simulation pipe connected\n");

    unsigned long arg;
    connected = true;

#ifdef _WIN32
    arg = 1;
    ioctlsocket(fd, FIONBIO, &arg);
#else
    fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
#endif

#ifdef SO_NOSIGPIPE
    arg = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

#ifdef TCP_NODELAY
    arg = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
#endif
}

void UsbHardwareMC::disconnect()
{
    if (fd >= 0) {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
    }

    if (connected) {
        printf("INFO: usb simulation pipe disconnect\n");
    }

    fd = -1;
    connected = false;
}

void UsbHardwareMC::processRxData()
{
    /*
     * Process received packets.
     * Complete packets are added to mBufferedINPackets.
     *
     * XXX: this does not currently handle incomplete packets.
     */

    uint8_t packetlen;
    int rxed = ::recv(fd, &packetlen, sizeof packetlen, 0);
    if (rxed <= 0) {
        disconnect();
        return;
    }

    RxPacket pkt;
    pkt.resize(packetlen - 1);

    rxed = ::recv(fd, &pkt[0], pkt.size(), 0);
    if (rxed > 0) {

        bufferedOUTPackets.push(pkt);
        Tasks::trigger(Tasks::UsbOUT);  // indicate that we have pending data

    } else if (errno != EAGAIN) {
        disconnect();
    }
}

uint16_t UsbHardwareMC::epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    /*
     * Pop a previously received packet into the caller's buf.
     */

    if (bufferedOUTPackets.empty()) {
        return 0;
    }

    const RxPacket &pkt = bufferedOUTPackets.front();
    len = MIN(len, pkt.size());
    memcpy(buf, &pkt[0], len);

    bufferedOUTPackets.pop();

    return len;
}

uint16_t UsbHardwareMC::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    /*
     * Write a packet directly to our socket.
     *
     * We copy the payload such that it's contiguous with our
     * small header so we can write it in a single send().
     */

    uint8_t pktbuf[64 + USB_HW_HDR_LEN];
    ASSERT(len <= 64);

    struct MsgPacket {
        uint8_t len;
        uint8_t payload[1];
    } *packet = (MsgPacket *) pktbuf;

    packet->len = len + USB_HW_HDR_LEN;
    memcpy(packet->payload, buf, len);

    int sent = ::send(fd, pktbuf, packet->len, 0);
    if (sent < 0) {
        fprintf(stderr, "send1 err: %s (%d)\n", strerror(errno), errno);
    }

    return len;
}

void UsbHardwareMC::work()
{
    /*
     * Run the select loop to pump our usb simulation socket.
     */

    if (!connected) {
        tryConnect();
        if (!connected) {
            return; // Can't do anything until we're connected.
        }
    }

    FD_SET(fd, &rfds);
    FD_SET(fd, &efds);
    FD_SET(fd, &wfds);

    int nfds = select(fd + 1, &rfds, &wfds, &efds, NULL);
    if (nfds <= 0) {
        return;
    }

    // Handle socket exceptions
    if (connected && FD_ISSET(fd, &efds)) {
        disconnect();
    }

    // more data to process?
    if (connected && FD_ISSET(fd, &rfds)) {
        processRxData();
    }

    // XXX: track write completion events?
}
