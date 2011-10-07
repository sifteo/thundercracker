/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
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
#include <SDL.h>
#include "network.h"

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

#define NETHUB_SET_ADDR         0x00
#define NETHUB_MSG              0x01
#define NETHUB_ACK              0x02
#define NETHUB_NACK             0x03

struct {
    struct addrinfo *addr;
    SDL_Thread *thread;
    SDL_sem *rx_sem;
    uint64_t rf_addr;
    int fd;
    int is_connected;
    int is_running;

    // Raw receive buffer
    int rx_count;
    uint8_t rx_buffer[1024];

    // Received packet buffer
    int rx_packet_len;
    uint64_t rx_addr;
    uint8_t rx_packet[256];
} net;


static void network_disconnect(void)
{
    if (net.fd >= 0) {
#ifdef _WIN32
        closesocket(net.fd);
#else
        close(net.fd);
#endif
    }

    net.fd = -1;
    net.is_connected = 0;
}   

static void network_tx_bytes(uint8_t *data, int len)
{
    if (net.fd < 0 || !net.is_connected)
        return;

    while (len > 0) {
        int ret = send(net.fd, data, len, 0);

        if (ret > 0) {
            len -= ret;
            data += ret;
        } else {
            network_disconnect();
            break;
        }
    }
}

static void network_addr_to_bytes(uint64_t addr, uint8_t *bytes)
{
    int i;
    for (i = 0; i < 8; i++) {
        bytes[i] = (uint8_t) addr;
        addr >>= 8;
    }
}

static uint64_t network_addr_from_bytes(uint8_t *bytes)
{
    uint64_t addr = 0;
    int i;
    for (i = 7; i >= 0; i--) {
        addr <<= 8;
        addr |= bytes[i];
    }
    return addr;
}

static void network_set_addr_internal(uint64_t addr)
{
    uint8_t packet[10] = { 8, NETHUB_SET_ADDR };
    network_addr_to_bytes(addr, &packet[2]);
    network_tx_bytes(packet, sizeof packet);
}

static void network_try_connect(void)
{
    if (net.fd < 0) {
        net.rx_count = 0;

        net.fd = socket(net.addr->ai_family, net.addr->ai_socktype, net.addr->ai_protocol);
        if (net.fd < 0)
            return;
    }

    if (!connect(net.fd, net.addr->ai_addr, net.addr->ai_addrlen)) {
        unsigned long arg;

        net.is_connected = 1;

#ifdef _WIN32
        arg = 1;
        ioctlsocket(net.fd, FIONBIO, &arg);
#else
        fcntl(net.fd, F_SETFL, O_NONBLOCK | fcntl(net.fd, F_GETFL));
#endif

#ifdef SO_NOSIGPIPE
        arg = 1;
        setsockopt(net.fd, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

#ifdef TCP_NODELAY
        arg = 1;
        setsockopt(net.fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
#endif

        if (net.rf_addr)
            network_set_addr_internal(net.rf_addr);
    } else {
        // Connection error, don't retry immediately
        SDL_Delay(300);
        network_disconnect();
    }
}

static void network_rx_into_buffer(void)
{
    int recv_len = 1;
    fd_set rfds;

    FD_ZERO(&rfds);
        
    while (net.is_running && net.is_connected) {
        int packet_len = 2 + (int)net.rx_buffer[0];
        uint8_t packet_type = net.rx_buffer[1];

        if (net.rx_count >= packet_len) {
            /*
             * We have a packet! Is it a message? We currently ignore
             * remote ACKs. (We're providing unidirectional flow
             * control, not consuming the remote end's flow control.)
             */
            
            if (packet_type == NETHUB_MSG) {
                static uint8_t ack[] = { 0, NETHUB_ACK };
                network_tx_bytes(ack, sizeof ack);

                // Make sure the packet buffer is available
                SDL_SemWait(net.rx_sem);

                net.rx_addr = network_addr_from_bytes(net.rx_buffer + 2);
                memcpy(net.rx_packet, net.rx_buffer + 10,
                       packet_len - 10);

                // This assignment transfers buffer ownership to the main thread
                net.rx_packet_len = packet_len - 10;
            }

            // Remove packet from buffer
            memmove(net.rx_buffer, net.rx_buffer + packet_len, net.rx_count - packet_len);
            net.rx_count -= packet_len;

        } else {
            /*
             * No packet buffered. Try to read more!
             *
             * We have the socket in nonblocking mode so that our
             * writes will not block, but here we do want a blocking
             * read.
             */

            FD_SET(net.fd, &rfds);
            select(net.fd + 1, &rfds, NULL, NULL, NULL);
            
            recv_len = recv(net.fd, net.rx_buffer + net.rx_count,
                            sizeof net.rx_buffer - net.rx_count, 0);
            if (recv_len <= 0) {
                if (recv_len == 0 || errno != EAGAIN)
                    network_disconnect();
                break;
            }

            net.rx_count += recv_len;
        }
    }
}

static int network_thread(void *param)
{
    while (net.is_running) {
        if (net.is_connected)
            network_rx_into_buffer();
        else
            network_try_connect();
    }
    return 0;
}

void network_init(const char *host, const char *port)
{
    struct addrinfo hints;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    memset(&net, 0, sizeof net);
    net.fd = -1;
    net.is_running = 1;
    net.rx_packet_len = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if (getaddrinfo(host, port, &hints, &net.addr)) {
        perror("getaddrinfo");
        exit(1);
    }

    // Buffer is initially available
    net.rx_sem = SDL_CreateSemaphore(1);

    net.thread = SDL_CreateThread(network_thread, NULL);
}

void network_exit(void)
{
    network_disconnect();
    freeaddrinfo(net.addr);

    net.is_running = 0;
    SDL_SemPost(net.rx_sem);
}

void network_tx(uint64_t addr, void *payload, int len)
{
    uint8_t buffer[512];

    if (len <= 255 - 8) {
        buffer[0] = len + 8;
        buffer[1] = NETHUB_MSG;
        network_addr_to_bytes(addr, buffer + 2);
        memcpy(buffer + 10, payload, len);

        network_tx_bytes(buffer, len + 10);
    }
}

void network_set_addr(uint64_t addr)
{
    if (net.rf_addr != addr) {
        net.rf_addr = addr;
        network_set_addr_internal(addr);
    }
}

int network_rx(uint64_t *addr, uint8_t payload[256])
{
    int len = net.rx_packet_len;

    if (len >= 0) {
        *addr = net.rx_addr;
        memcpy(payload, net.rx_packet, len);
        net.rx_packet_len = -1;
        SDL_SemPost(net.rx_sem);
    }
 
    return len;
}

