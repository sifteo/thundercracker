/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This network simulation assumes we're going to be checking for RX
 * packets very frequently, as part of the hardware simulation. To keep
 * the syscall overhead from overwhelming us, we have a separate thread
 * that spends its time waiting on I/O. When a packet is received, it's
 * buffered, then the main thread uses a semaphore to wake up the waiting
 * I/O thread after the buffer is available.
 *
 * We use SDL for multithreading primitives here, just so we don't
 * have to introduce any new dependencies.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <SDL/SDL.h>
#include "cube_network.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#endif

namespace Cube {


void NetworkClient::disconnect()
{
    if (fd >= 0) {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
    }

    fd = -1;
    is_connected = 0;
}   

void NetworkClient::txBytes(uint8_t *data, int len)
{
    if (fd < 0 || !is_connected)
        return;

    while (len > 0) {
        int ret = send(fd, data, len, 0);

        if (ret > 0) {
            len -= ret;
            data += ret;
        } else {
            disconnect();
            break;
        }
    }
}

void NetworkClient::addrToBytes(uint64_t addr, uint8_t *bytes)
{
    int i;
    for (i = 0; i < 8; i++) {
        bytes[i] = (uint8_t) addr;
        addr >>= 8;
    }
}

uint64_t NetworkClient::addrFromBytes(uint8_t *bytes)
{
    uint64_t addr = 0;
    int i;
    for (i = 7; i >= 0; i--) {
        addr <<= 8;
        addr |= bytes[i];
    }
    return addr;
}

void NetworkClient::setAddrInternal(uint64_t addr)
{
    uint8_t packet[10] = { 8, NETHUB_SET_ADDR };
    addrToBytes(addr, &packet[2]);
    txBytes(packet, sizeof packet);
}

void NetworkClient::tryConnect()
{
    if (fd < 0) {
        rx_count = 0;

        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0)
            return;
    }

    if (!connect(fd, addr->ai_addr, addr->ai_addrlen)) {
        unsigned long arg;

        is_connected = 1;

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

        if (rf_addr)
            setAddrInternal(rf_addr);
    } else {
        // Connection error, don't retry immediately
        SDL_Delay(300);
        disconnect();
    }
}

void NetworkClient::rxIntoBuffer()
{
    int recv_len = 1;
    fd_set rfds;

    FD_ZERO(&rfds);
        
    while (is_running && is_connected) {
        int packet_len = 2 + (int)rx_buffer[0];
        uint8_t packet_type = rx_buffer[1];

        if (rx_count >= packet_len) {
            /*
             * We have a packet! Is it a message? We currently ignore
             * remote ACKs. (We're providing unidirectional flow
             * control, not consuming the remote end's flow control.)
             */
            
            if (packet_type == NETHUB_MSG) {
                static uint8_t ack[] = { 0, NETHUB_ACK };
                txBytes(ack, sizeof ack);

                // Make sure the packet buffer is available
                SDL_SemWait(rx_sem);

                rx_addr = addrFromBytes(rx_buffer + 2);
                memcpy(rx_packet, rx_buffer + 10,
                       packet_len - 10);

                // This assignment transfers buffer ownership to the main thread
                rx_packet_len = packet_len - 10;
            }

            // Remove packet from buffer
            memmove(rx_buffer, rx_buffer + packet_len, rx_count - packet_len);
            rx_count -= packet_len;

        } else {
            /*
             * No packet buffered. Try to read more!
             *
             * We have the socket in nonblocking mode so that our
             * writes will not block, but here we do want a blocking
             * read.
             */

            FD_SET(fd, &rfds);
            select(fd + 1, &rfds, NULL, NULL, NULL);
            
            recv_len = recv(fd, rx_buffer + rx_count,
                            sizeof rx_buffer - rx_count, 0);
            if (recv_len <= 0) {
                if (recv_len == 0 || errno != EAGAIN)
                    disconnect();
                break;
            }

            rx_count += recv_len;
        }
    }
}

int NetworkClient::threadFn(void *param)
{
    NetworkClient *self = (NetworkClient *) param;

    while (self->is_running) {
        if (self->is_connected)
            self->rxIntoBuffer();
        else
            self->tryConnect();
    }

    return 0;
}

void NetworkClient::init(const char *host, const char *port)
{
    struct addrinfo hints;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    rf_addr = 0;
    fd = -1;
    is_connected = 0;
    is_running = 1;
    rx_packet_len = -1;
    rx_count = 0;
    rx_packet_len = 0;
    rx_addr = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(host, port, &hints, &addr);

    // Buffer is initially available
    rx_sem = SDL_CreateSemaphore(1);

    thread = SDL_CreateThread(threadFn, this);
}

void NetworkClient::exit(void)
{
    disconnect();

    is_running = 0;
    SDL_SemPost(rx_sem);
    SDL_WaitThread(thread, NULL);

    freeaddrinfo(addr);
}

void NetworkClient::tx(uint64_t addr, void *payload, int len)
{
    uint8_t buffer[512];

    if (len <= 255 - 8) {
        buffer[0] = len + 8;
        buffer[1] = NETHUB_MSG;
        addrToBytes(addr, buffer + 2);
        memcpy(buffer + 10, payload, len);

        txBytes(buffer, len + 10);
    }
}

void NetworkClient::setAddr(uint64_t addr)
{
    if (rf_addr != addr) {
        rf_addr = addr;
        setAddrInternal(addr);
    }
}

int NetworkClient::rx(uint64_t *addr, uint8_t payload[256])
{
    int len = rx_packet_len;

    if (len >= 0) {
        *addr = rx_addr;
        memcpy(payload, rx_packet, len);
        rx_packet_len = -1;
        SDL_SemPost(rx_sem);
    }
 
    return len;
}


};  // namespace Cube
