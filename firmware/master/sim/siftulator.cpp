/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementation of Radio and SysTime based on synchronous
 * co-simulation with tc-siftulator.
 *
 * When we connect to tc-siftulator, it becomes a synchronous
 * emulator.  It won't run unless we've given it a radio event,
 * timestamped in the future, to run toward and dispatch. When it does
 * so, it synchronously sends us back the corresponding ACK.
 *
 * XXX: We aren't using threads or signals here (for portability),
 *      so we can only invoke ISR delegates from within halt().
 *      On real hardware, they can be invoked any time we aren't
 *      running another ISR.
 *
 * XXX: This is cheesy network code. Intended to be temporary,
 *      until Siftulator and the simulated master runtime live in
 *      the same binary. (After we have a runtime...)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <winsock2.h>
#   ifdef __MINGW32__
#       undef _WIN32_WINNT
#       define _WIN32_WINNT 0x0501 // redefine as WinXP so we have access to getaddrinfo
#   endif
#   include <ws2tcpip.h>
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#endif

#include "radio.h"
#include "systime.h"

/*
 * Network protocol constants
 */

static const char *HOST = "127.0.0.1";
static const char *PORT = "2404";
static const uint8_t PAYLOAD_MAX = 32;
static const uint32_t TICK_HZ = 16000000;
static const uint32_t TICKS_PER_PACKET = 7200;   // 450us, minimum packet period
static const uint32_t MAX_RETRIES = 150;         // Simulates (hardware * software) retries

struct RadioData {
    uint8_t pid;
    uint8_t len;
    uint8_t payload[PAYLOAD_MAX];
};

struct RXPacket {
    uint8_t ack;
    RadioData p;
};

struct TXPacket {
    uint64_t tickDelta;
    uint8_t addr[5];
    uint8_t reserved[2];
    uint8_t channel;
    RadioData p;
};

/*
 * Private static data, doesn't need to be known outside this file.
 */

static struct Siftulator {
    struct addrinfo *addr;
    fd_set rfds, wfds, efds;
    int fd;

    bool isConnected;
    bool ackPending;

    uint64_t simTicks;
    uint32_t retriesLeft;
} self;


void SysTime::init()
{
    self.simTicks = 0;

}

SysTime::Ticks SysTime::ticks()
{
    /*
     * Our TICK_HZ divides easily into nanoseconds (62.5 ns at 16 MHz)
     * so we can do this conversion using fixed-point math quite easily.
     *
     * This does it in 64-bit math, with 60.4 fixed-point.
     */

    return (self.simTicks * hzTicks(TICK_HZ / 16)) >> 4;
}

void Radio::open()
{
    /*
     * Initialize the radio device, and try to connect. We may not be
     * able to connect right away, though, or we may get
     * disconnected. We'll try to reconnect as necessary.
     */

    struct addrinfo hints;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    memset(&self, 0, sizeof self);
    self.fd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(HOST, PORT, &hints, &self.addr);
}

static void Siftulator_disconnect()
{
    fprintf(stderr, "Warning: Siftulator disconnected\n");
    
    if (self.fd >= 0) {
#ifdef _WIN32
        closesocket(self.fd);
#else
        close(self.fd);
#endif
    }

    self.fd = -1;
    self.isConnected = false;
}

static void Siftulator_tryConnect()
{
    if (self.fd < 0) {
        FD_ZERO(&self.rfds);
        FD_ZERO(&self.wfds);
        FD_ZERO(&self.efds);

        self.ackPending = false;

        self.fd = socket(self.addr->ai_family, self.addr->ai_socktype,
                         self.addr->ai_protocol);
        if (self.fd < 0)
            return;
    }

    if (connect(self.fd, self.addr->ai_addr, self.addr->ai_addrlen)) {
        Siftulator_disconnect();
    } else {
        unsigned long arg;

        self.isConnected = true;

#ifdef _WIN32
        arg = 1;
        ioctlsocket(self.fd, FIONBIO, &arg);
#else
        fcntl(self.fd, F_SETFL, O_NONBLOCK | fcntl(self.fd, F_GETFL));
#endif

#ifdef SO_NOSIGPIPE
        arg = 1;
        setsockopt(self.fd, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

        arg = 1;
        setsockopt(self.fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
    }
}

static void Siftulator_send()
{
    /*
     * Send either a new packet or a retry of an old packet.
     */

    static TXPacket packet;

    if (!self.retriesLeft) {
        // Produce a new packet, with a new PID.

        PacketTransmission ptx;

        memset(&packet, 0, sizeof packet);
        ptx.packet.bytes = packet.p.payload;
        RadioManager::produce(ptx);

        packet.channel = ptx.dest->channel;
        packet.p.len = ptx.packet.len;
        memcpy(packet.addr, ptx.dest->id, sizeof packet.addr);

        packet.p.pid++;
        self.retriesLeft = MAX_RETRIES;
    }

    // XXX: Okay to hardcode only because we're sending packets continuously now.
    packet.tickDelta = TICKS_PER_PACKET; 
    self.simTicks += TICKS_PER_PACKET;

    int ret = send(self.fd, (const char *)&packet, sizeof packet, 0);

    if (ret < 0) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Error: Siftulator write error (%d)\n", errno);
            Siftulator_disconnect();
        }
    } else if (ret != sizeof packet) {
        fprintf(stderr, "Error: Unhandled partial write to Siftulator (%d bytes)\n", ret);
        Siftulator_disconnect();
    } else {
        self.ackPending = true;
    }
}

static void Siftulator_recv()
{
    RXPacket packet;
    PacketBuffer buf;

    int ret = recv(self.fd, (char *)&packet, sizeof packet, 0);

    if (ret < 0) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Error: Siftulator read error (%d)\n", errno);
            Siftulator_disconnect();
        }
    } else if (ret != sizeof packet) {
        fprintf(stderr, "Error: Siftulator packet size %d unexpected\n", ret);
        Siftulator_disconnect();
    } else {

        if (packet.ack) {
            self.retriesLeft = 0;

            /*
             * We're allowed to send an empty ackWithPacket(), but our
             * real radio driver makes a distinction between these
             * cases, for a small efficiency boost. So mimic the
             * hardware.
             */

            if (packet.p.len) {
                buf.len = packet.p.len;
                buf.bytes = packet.p.payload;
                RadioManager::ackWithPacket(buf);
            } else {
                RadioManager::ackEmpty();
            }

        } else {
            // only timeout on transition to zero
            if (self.retriesLeft >= 1) {
                if (--self.retriesLeft == 0) {
                    RadioManager::timeout();
                }
            }
        }
        
        self.ackPending = false;
    }
}


void Radio::halt()
{
    /*
     * In our implementation, halt() runs the select loop which
     * actually pumps data between our local buffers and a TCP socket.
     */

    static bool inHalt = false;
    assert(inHalt == false);
    inHalt = true;

    if (!self.isConnected) {
        Siftulator_tryConnect();

        if (!self.isConnected) {
            /*
             * Can't do anything until we get our siftulator back. Sleep a bit and try again.
             * Advance time manually, until we get our connection back.
             */

            self.simTicks += TICK_HZ / 10;
            #ifdef _WIN32
            Sleep(100);
            #else
            usleep(100000);
            #endif
        }

    } else {
        if (self.ackPending) {
            // Receiving
            FD_SET(self.fd, &self.efds);
            FD_SET(self.fd, &self.rfds);
            int nfds = select(self.fd + 1, &self.rfds, &self.wfds, &self.efds, NULL);
            if (nfds > 0) {
                if (self.isConnected && FD_ISSET(self.fd, &self.efds))
                    Siftulator_disconnect();        
                if (self.isConnected && FD_ISSET(self.fd, &self.rfds))
                    Siftulator_recv();
            }

        } else {
            // Sending
            Siftulator_send();
        }
    }

    inHalt = false;
}
