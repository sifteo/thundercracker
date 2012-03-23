/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#include <stdint.h>


class GDBServer {
public:
    static void start(int port);
    static void stop();

    void threadEntry();
    static GDBServer instance;

private:
    GDBServer() {}

    int port;
    int clientFD;
    int packetState;
    bool running;

    unsigned txBufferLen;
    unsigned rxPacketLen;
    uint8_t rxChecksum;
    uint8_t txChecksum;
    char txBuffer[2048];
    char rxPacket[1024];

    void eventLoop();
    void handleClient();
    void resetPacketState();
    void rxBytes(char *bytes, int len);
    
    bool packetStartsWith(const char *str);
    void handlePacket();
    
    void txFlush();
    void txByte(uint8_t byte);
    void txString(const char *str);
    void txPacketBegin();
    void txPacketEnd();
    void txPacketString(const char *str);
    void txHexByte(uint8_t byte);

    static char digitToHex(int i);
    static int digitFromHex(char c);
};

#endif  // GDB_SERVER_H
