/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * XXX: This is the cheesiest network code ever. But it's temporary- as soon
 *      as we have a runtime, I suspect we'll be running the master cube firmware
 *      in-process. If not, we can come up with a better network protocol. This one
 *      is *not* intended to be high-performance, it's intended to provide accurate
 *      simulation, keeping the master and the cubes in sync.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "system_network.h"
#include "system.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define WINVER WindowsXP
#   define _WIN32_WINNT 0x502
#   include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <netinet/tcp.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#endif


void SystemNetwork::init()
{
    unsigned long arg;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    listenFD = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(listenFD, (struct sockaddr *)&addr, sizeof addr) < 0) {
        fprintf(stderr, "Error: Can't bind to simulator port!\n");
    }

    /*
     * We use blocking I/O for the client socket, but non-blocking for
     * the listener socket.
     */
#ifdef _WIN32
    arg = 1;
    ioctlsocket(listenFD, FIONBIO, &arg);
#else
    fcntl(listenFD, F_SETFL, O_NONBLOCK | fcntl(listenFD, F_GETFL));
#endif

    arg = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof arg);

    if (listen(listenFD, 1) < 0) {
        fprintf(stderr, "Error: Can't listen on simulator socket!\n");
    }
}

void SystemNetwork::disconnect(int &fd)
{
    if (fd >= 0) {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
    }
    fd = -1;
}

void SystemNetwork::exit()
{
    disconnect(clientFD);
    disconnect(listenFD);
}

void SystemNetwork::tx(TXPacket &packet)
{
    /* Send back a response. No-op if we have no client. */

    if (clientFD >= 0) {
        if (write(clientFD, &packet, sizeof packet) != sizeof packet)
            disconnect(clientFD);
    }
}

bool SystemNetwork::rx(RXPacket &packet)
{
    /*
     * Try to read a response. If we have no client, or we lose a
     * client, returns false. If we do have a client, blocks until a
     * packet is available, then returns true.
     */

    if (clientFD < 0) {
        struct sockaddr_in addr;
        socklen_t addrSize = sizeof addr;
        unsigned long arg;
        
        clientFD = accept(listenFD, (struct sockaddr *) &addr, &addrSize);

#ifdef SO_NOSIGPIPE
        arg = 1;
        setsockopt(clientFD, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

        arg = 1;
        setsockopt(clientFD, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
    }

    if (clientFD >= 0 && read(clientFD, &packet, sizeof packet) == sizeof packet)
        return true;

    disconnect(clientFD);
    return false;
}


uint64_t SystemNetwork::timerNext(System &sys)
{
    /*
     * Support co-simulation with an external master-cube process.
     * If we have no active connection, we'll just poll for a
     * client every so often. But if we do have a connection, it
     * will be able to synchronously inject radio packets at
     * particular simulation timesteps.
     */

    do {
        if (packetIsWaiting) {
            // The last packet we received is now ready to send.
            packetIsWaiting = false;
            deliverPacket(sys, nextPacket);
        }

        if (!rx(nextPacket))
            return VirtualTime::usec(POLL_MS);

        packetIsWaiting = true;

    } while (nextPacket.tickDelta == 0);

    return nextPacket.tickDelta;
}

void SystemNetwork::deliverPacket(System &sys, RXPacket &packet)
{
    /*
     * Dispatch this received packet back to a cube. Any cube. Or no cube.
     */

    TXPacket reply;
    memset(&reply, 0, sizeof reply);

    for (unsigned i = 0; i < sys.opt_numCubes; i++) {
        Cube::Radio &radio = sys.cubes[i].spi.radio;

        if (radio.getPackedRXAddr() == packet.address) {
            reply.ack = true;
            radio.handlePacket(packet.p, reply.p);
            break;
        }
    }

    tx(reply);
}
