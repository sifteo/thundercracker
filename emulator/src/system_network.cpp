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


void SystemNetwork::init(const VirtualTime *vtime)
{
    unsigned long arg;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0x0100007f;
    addr.sin_port = htons(PORT);

    listenFD = socket(AF_INET, SOCK_STREAM, 0);

    arg = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, (const char *)&arg, sizeof arg);

    if (bind(listenFD, (struct sockaddr *)&addr, sizeof addr) < 0) {
        fprintf(stderr, "Error: Can't bind to simulator port!\n");
    }

    setNonBlock(listenFD);
    if (listen(listenFD, 1) < 0) {
        fprintf(stderr, "Error: Can't listen on simulator socket!\n");
    }

    deadline.init(vtime);
    deadline.setRelative(0);
}

void SystemNetwork::setNonBlock(int fd)
{
#ifdef _WIN32
    unsigned long arg = 1;
    ioctlsocket(listenFD, FIONBIO, &arg);
#else
    fcntl(listenFD, F_SETFL, fcntl(listenFD, F_GETFL) | O_NONBLOCK);
#endif
}

void SystemNetwork::disconnect(int &fd)
{
    fprintf(stderr, "NET: Client disconnected\n");

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
        if (send(clientFD, (const char *)&packet, sizeof packet, 0) != sizeof packet)
            disconnect(clientFD);
    }
}

bool SystemNetwork::rx(RXPacket &packet)
{
    /*
     * Try to read a response. If we have no client, or we lose a
     * client, returns false. If we do have a client, blocks until a
     * packet is available, then returns true.
     *
     * We want to make this blocking, but on some platforms (Mac OS,
     * *cough*) we can't reliably make an accepted socket blocking
     * again if the listener was nonblocking. So, select() first.
     */

    static fd_set efds, rfds;

    if (clientFD < 0) {
        struct sockaddr_in addr;
        socklen_t addrSize = sizeof addr;
        
        clientFD = accept(listenFD, (struct sockaddr *) &addr, &addrSize);

        if (clientFD >= 0) {
            unsigned long arg;

            FD_ZERO(&efds);
            FD_ZERO(&rfds);

            setNonBlock(clientFD);

#ifdef SO_NOSIGPIPE
            arg = 1;
            setsockopt(clientFD, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

            arg = 1;
            setsockopt(clientFD, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
        }
    }

    if (clientFD >= 0) {

        FD_SET(clientFD, &rfds);
        FD_SET(clientFD, &efds);
        select(clientFD + 1, &rfds, NULL, &efds, NULL);

        int ret = recv(clientFD, (char *)&packet, sizeof packet, 0);

        if (ret < 0) {
            if (errno != EAGAIN) {
                fprintf(stderr, "NET: Read error (%d)\n", errno);
                disconnect(clientFD);
            }
        } else if (ret != sizeof packet) {
            if (ret > 0) {
                fprintf(stderr, "NET: Unexpected packet size, %d bytes\n", ret);
            }
            disconnect(clientFD);
        } else {
            return true;
        }
    }

    return false;
}


NEVER_INLINE void SystemNetwork::deadlineWork(System &sys)
{
    /*
     * Support co-simulation with an external master-cube process.
     * If we have no active connection, we'll just poll for a
     * client every so often. But if we do have a connection, it
     * will be able to synchronously inject radio packets at
     * particular simulation timesteps.
     */

    deadline.reset();

    do {
        if (packetIsWaiting) {
            // The last packet we received is now ready to send.
            packetIsWaiting = false;
            deliverPacket(sys, nextPacket);
        }

        if (!rx(nextPacket)) {
            // Poll for a connection to be established
            deadline.setRelative(VirtualTime::msec(POLL_MS));
            return;
        }

        packetIsWaiting = true;

    } while (nextPacket.tickDelta == 0);

    deadline.setRelative(nextPacket.tickDelta);
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
            reply.ack = radio.handlePacket(packet.p, reply.p);
            break;
        }
    }

    tx(reply);
}
