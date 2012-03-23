/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "gdbserver.h"
#include <string.h>
#include <stdio.h>

#define LOG_PREFIX  "GDB Server: "

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
#   include <pthread.h>
#   include <errno.h>
#endif

GDBServer GDBServer::instance;


#ifdef _WIN32
static DWORD WINAPI GDBServerThread(LPVOID param)
#else
static void *GDBServerThread(void *param)
#endif
{
    /*
     * This is a thin wrapper around threadEntry(), which lets us have
     * the proper platform-specific calling signatures without pulling
     * all these giant OS headers into gdbserver.h
     */

    GDBServer::instance.threadEntry();
    return 0;
}

void GDBServer::start(int port)
{
    /*
     * Spawn a thread which listens for connections from GDB
     */

    instance.port = port;
    instance.running = true;

#ifdef _WIN32
    CreateThread(NULL, 0, GDBServerThread, 0, 0, NULL);
#else
    pthread_t t;
    pthread_create(&t, NULL, GDBServerThread, 0);
#endif
}

void GDBServer::stop()
{
    /*
     * Ask the background thread to stop at its next convenience,
     * asynchronously.
     */

    instance.running = false;
}

void GDBServer::threadEntry()
{
    /*
     * Thread for the GDB server. We start listening for incoming TCP
     * connections, and invoke handleClient() for each client connection.
     * Only one client can be connected at a time.
     */

    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0x0100007f;
    addr.sin_port = htons(port);

    int listenFD = socket(AF_INET, SOCK_STREAM, 0);

    unsigned long arg = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, (const char *)&arg, sizeof arg);

    if (bind(listenFD, (struct sockaddr *)&addr, sizeof addr) < 0) {
        fprintf(stderr, LOG_PREFIX "Can't bind to port!\n");
        return;
    }

    if (listen(listenFD, 1) < 0) {
        fprintf(stderr, LOG_PREFIX "Can't listen on socket\n");
        return;
    }
    
    fprintf(stderr, LOG_PREFIX "Listening on port %d. Connect with "
        "\"target remote localhost:%d\"\n", port, port);

    while (running) {
        struct sockaddr_in addr;
        socklen_t addrSize = sizeof addr;
        clientFD = accept(listenFD, (struct sockaddr *) &addr, &addrSize);
        if (clientFD < 0)
            break;

        handleClient();
        close(clientFD);
    }
    
    close(listenFD);
}

void GDBServer::handleClient()
{
    /*
     * This is the main per-client loop. Listen for bytes, which we send
     * to rxBytes() to be reassembled into GDB packets.
     */

    fprintf(stderr, LOG_PREFIX "Client connected\n");

    unsigned long arg;
    #ifdef SO_NOSIGPIPE
        arg = 1;
        setsockopt(clientFD, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
    #endif
    arg = 1;
    setsockopt(clientFD, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);

    resetPacketState();

    while (1) {
        char rxBuffer[4096];
        int ret = recv(clientFD, rxBuffer, sizeof rxBuffer, 0);

        if (ret < 0) {
            if (errno != EAGAIN) {
                fprintf(stderr, LOG_PREFIX "Read error (%d)\n", errno);
                return;
            }
        }
        
        if (ret > 0)
            rxBytes(rxBuffer, ret);

        txFlush();
    }
}

void GDBServer::txByte(uint8_t byte)
{
    /*
     * Append one byte to the transmit buffer. If it's full, flush
     * it to the network stack.
     */

    if (txBufferLen == sizeof txBuffer)
        txFlush();
    txBuffer[txBufferLen++] = byte;
    txChecksum += byte;
}

void GDBServer::txFlush()
{
    /*
     * Send the contents of txBuffer, and empty it.
     */

    if (txBufferLen) {
        send(clientFD, txBuffer, txBufferLen, 0);
        txBufferLen = 0;
    }
}

void GDBServer::resetPacketState()
{
    /*
     * Reset any per-packet state. This is used when a new client is connected,
     * or when a packet is completed.
     */

    packetState = 0;
    txBufferLen = 0;
    rxPacketLen = 0;
    rxChecksum = 0;
}

void GDBServer::rxBytes(char *bytes, int len)
{
    /*
     * Gather raw received bytes into fully framed GDB packets, and dispatch
     * them to rxPacket(). This buffers partial packets in rxPacketBuf[].
     */
    
    enum {
        S_NOT_IN_PACKET = 0,
        S_PAYLOAD,
        S_CHECKSUM_0,
        S_CHECKSUM_1,
    };
    
    for (int i = 0; i < len; i++) {
        char byte = bytes[i];
        switch (packetState) {

        case S_NOT_IN_PACKET:
            if (byte == '$') {
                resetPacketState();
                packetState = S_PAYLOAD;
            }
            break;

        case S_PAYLOAD:
            if (byte == '#') {
                // Begin checksum footer
                packetState = S_CHECKSUM_0;
            } else if (rxPacketLen < sizeof rxPacket - 1) {
                // Normal payload byte. Part of the checksum.
                rxChecksum += byte;
                rxPacket[rxPacketLen++] = byte;
            } else {
                // Overflowed receive buffer. Discard the packet.
                // Note the -1 above; we leave room for a NUL terminator.
                packetState = S_NOT_IN_PACKET;
                txByte('-');
            }
            break;

        case S_CHECKSUM_0:
            // First (most significant) checksum digit
            if ((rxChecksum >> 4) == digitFromHex(byte)) {
                packetState = S_CHECKSUM_1;
            } else {
                packetState = S_NOT_IN_PACKET;
                txByte('-');
            }
            break;

        case S_CHECKSUM_1:
            // Second (least significant) checksum digit
            packetState = S_NOT_IN_PACKET;
            if ((rxChecksum & 0xF) == digitFromHex(byte)) {
                // Successfully received
                txByte('+');
                
                // NUL terminate and dispatch.
                rxPacket[rxPacketLen] = 0;
                handlePacket();
            } else {
                txByte('-');
            }
            break;
        }
    }
}

char GDBServer::digitToHex(int i)
{
    static const char lut[] = "0123456789abcdef";
    return lut[i & 0xF];
}

int GDBServer::digitFromHex(char c)
{
    if (c < '0')    return -1;
    if (c <= '9')   return c - '0';
    if (c < 'A')    return -1;
    if (c <= 'F')   return c - 'A' + 10;
    if (c < 'a')    return -1;
    if (c <= 'f')   return c - 'a' + 10;
    return -1;
}

bool GDBServer::packetStartsWith(const char *str)
{
    return 0 == strncmp(rxPacket, str, strlen(str));
}

void GDBServer::txPacketBegin()
{
    txByte('$');
    txChecksum = 0;
}

void GDBServer::txPacketEnd()
{
    uint8_t sum = txChecksum;
    txByte('#');
    txHexByte(sum);
}

void GDBServer::txHexByte(uint8_t byte)
{
    txByte(digitToHex(byte >> 4));
    txByte(digitToHex(byte));
}

void GDBServer::txString(const char *str)
{
    char c;
    while ((c = *str)) {
        txByte(c);
        str++;
    }
}

void GDBServer::txPacketString(const char *str)
{
    txPacketBegin();
    txString(str);
    txPacketEnd();
}

void GDBServer::handlePacket()
{
    switch (rxPacket[0]) {

    case 'q':
        if (packetStartsWith("qSupported")) return txPacketString("PacketSize=512");
        if (packetStartsWith("qOffsets"))   return txPacketString("Text=0;Data=0;Bss=0");
        break;

    case '?':
        // Why did we last stop?
        return txPacketString("S05");

    case 'g':
        // Read registers
        return txPacketString("00000000");

    case 'G':
        // Write registers
        return txPacketString("OK");

    case 'm':
        // Read memory
        return txPacketString("00000000");

    case 'M':
        // Write memory
        return txPacketString("OK");

    case 'c':
        // Continue
        return;

    case 's':
        // Single step
        return;

    }

    // Unsupported packet; empty reply
    txPacketString("");
}