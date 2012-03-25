/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#include <stdint.h>
#include "svm.h"
#include "tinythread.h"

class ELFDebugInfo;


class GDBServer {
public:
    typedef uint32_t (*MessageCallback)(const uint32_t *cmd, uint32_t cmdWords, uint32_t *reply);
    
    static void start(int port);
    static void stop();

    static void setDebugInfo(const ELFDebugInfo *info);
    static void setMessageCallback(MessageCallback cb);

private:
    GDBServer() {}
    static GDBServer instance;

    tthread::thread *thread;
    const ELFDebugInfo *debugInfo;
    MessageCallback messageCb;

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

    uint32_t msgCmd[Svm::Debugger::MAX_CMD_WORDS];
    uint32_t msgReply[Svm::Debugger::MAX_REPLY_WORDS];

    static void threadEntry(void *param);
    void threadMain();
    void eventLoop();
    void handleClient();
    void resetPacketState();
    void rxBytes(char *bytes, int len);
    
    bool packetStartsWith(const char *str);
    void handlePacket();
    uint32_t message(uint32_t words);
    
    void txFlush();
    void txByte(uint8_t byte);
    void txString(const char *str);
    void txPacketBegin();
    void txPacketEnd();
    void txPacketString(const char *str);
    void txHexByte(uint8_t byte);
    void txHexWord(uint32_t word);

    static char digitToHex(int i);
    static int digitFromHex(char c);
    
    void debugBreak();
    void replyToRegisterRead();
    bool readMemory(uint32_t addr, uint8_t *buffer, uint32_t bytes);
};

#endif  // GDB_SERVER_H
