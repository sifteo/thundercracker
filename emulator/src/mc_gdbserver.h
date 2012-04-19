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
    bool waitingForStop;

    unsigned txBufferLen;
    unsigned rxPacketLen;
    uint8_t rxChecksum;
    uint8_t txChecksum;
    char txBuffer[2048];
    char rxPacket[1024];

    uint32_t msgCmd[Svm::Debugger::MAX_CMD_WORDS];
    uint32_t msgReply[Svm::Debugger::MAX_REPLY_WORDS];

    static const unsigned GDB_BREAKPOINTS = Svm::Debugger::NUM_BREAKPOINTS;
    uint32_t breakpoints[GDB_BREAKPOINTS];

    // Register format
    static const unsigned NUM_GDB_REGISTERS = 26;
    int regGDBtoSVM(uint32_t r);
    static uint32_t findRegisterInPacket(uint32_t bitmap, uint32_t svmReg);

    static void threadEntry(void *param);
    void threadMain();
    void eventLoop();
    void handleClient();
    void resetPacketState();
    static void setNonBlock(int fd);
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
    
    uint8_t rxByte(uint32_t &offset);
    uint8_t rxHexByte(uint32_t &offset);
    uint32_t rxHexWord(uint32_t &offset);

    static char digitToHex(int i);
    static int digitFromHex(char c);
    
    void debugBreak();
    void pollForStop();

    void sendBreakpoints(uint32_t bitmap);
    void clearBreakpoints();
    bool insertBreakpoint(uint32_t addr);
    void removeBreakpoint(uint32_t addr);

    bool readMemory(uint32_t addr, uint8_t *buffer, uint32_t bytes);
    bool writeMemory(uint32_t addr, uint32_t bytes, uint32_t packetOffset);
};

#endif  // GDB_SERVER_H
