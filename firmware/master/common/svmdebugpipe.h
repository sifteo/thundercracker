/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Debugger bridge for SVM. This is the connection between debug-related
 * stubs in the SVM runtime, and a host system.
 */

#ifndef SVM_DEBUGPIPE_H
#define SVM_DEBUGPIPE_H

#include <stdint.h>
#include <inttypes.h>

#include "svm.h"
#include "flashlayer.h"

#ifdef SIFTEO_SIMULATOR
#   include <string>
#endif

using namespace Svm;


/**
 * Log messages are small binary packets, prefixed with a 32-bit word
 * that describes the type of packet and its length.
 */

class SvmLogTag {
public:
    SvmLogTag(uint32_t tag) : tag(tag) {}
    SvmLogTag(const SvmLogTag &tag) : tag(tag.getValue()) {}
    SvmLogTag(const SvmLogTag &tag, uint32_t param)
        : tag((tag.getValue() & 0xff000000) | param) {}

    inline uint32_t getParam() const {
        return tag & 0xffffff;
    }
    
    inline uint32_t getArity() const {
        return (tag >> 24) & 7;
    }

    inline uint32_t getType() const {
        return tag >> 27;
    }
    
    inline uint32_t getValue() const {
        return tag;
    }
    
private:
    uint32_t tag;
};


/**
 * Debugging interfaces between the SVM runtime and the outside world
 */

class SvmDebugPipe {
public:
    SvmDebugPipe();    // Do not implement

    static const unsigned LOG_BUFFER_WORDS = 7;
    static const unsigned LOG_BUFFER_BYTES = LOG_BUFFER_WORDS * sizeof(uint32_t);

    /**
     * Log messages are emitted in two steps:
     *
     *   1. logReserve() allocates space in a USB buffer, blocking if
     *      no space is yet available.
     *
     *   2. logCommit() indicates that we've filled the buffer, provides
     *      the actual length of the resulting data, and signals the hardware
     *      that this message may be transmitted to the USB host now.
     */
    static uint32_t *logReserve(SvmLogTag tag);
    static void logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes);

    struct DebuggerMsg {
        uint32_t *cmd;
        uint32_t *reply;
        uint32_t cmdWords;
        uint32_t replyWords;
    };
    
    /**
     * Debugger messages are handled any time that, simultaneously, an
     * incoming message is available and an outgoing buffer with
     * SvmDebuggerMsg::MAX_REPLY_WORDS 32-bit words of space is available.
     *
     * The debugger polls for messages by calling debuggerMsgAccept.
     * If both a message and a reply buffer are available, it returns
     * true and fills in 'msg'. When the debugger finishes processing the
     * message, it writes the appropriate reply into memory and calls
     * debuggerMsgFinish().
     */
    static bool debuggerMsgAccept(DebuggerMsg &msg);
    static void debuggerMsgFinish(DebuggerMsg &msg);

    /**
     * Returns true if the debugger has handled the fault, false if it
     * remains unhandled and the runtime should take action on it.
     */
    static bool fault(FaultCode code);

    static void setSymbolSourceELF(const FlashRange &elf);

#ifdef SIFTEO_SIMULATOR
    static std::string formatAddress(uint32_t address);
    static std::string formatAddress(void *address);
#endif
};


#endif // SVM_DEBUGPIPE_H
