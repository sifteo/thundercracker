/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Debug support for SVM. On the simulator, these implementations can be
 * luxurious and spacious. On real hardware, we want to minimally proxy debug
 * state to and from the host machine over USB, where we offload as much work
 * as possible.
 */

#ifndef SVM_DEBUG_H
#define SVM_DEBUG_H

#include <stdint.h>
#include <inttypes.h>

#include "svm.h"
#include "flashlayer.h"

#ifdef SIFTEO_SIMULATOR
#   include <string>
#endif

using namespace Svm;


/**
 * Debugger messages are command/response pairs which are sent from a
 * host to the SVM runtime. All debugger messages are formatted as a
 * bounded-length packet made up of 32-bit words.
 */

namespace SvmDebuggerMsg {
    /*
     * Symmetric maximum lengths. Long enough for all 14 registers plus a
     * command word. Leaves room for one header word before we fill up a
     * 64-byte USB packet.
     */
    const uint32_t MAX_CMD_WORDS = 15;
    const uint32_t MAX_REPLY_WORDS = 15;

    const uint32_t MAX_CMD_BYTES = MAX_CMD_WORDS * sizeof(uint32_t);
    const uint32_t MAX_REPLY_BYTES = MAX_REPLY_WORDS * sizeof(uint32_t);

    enum MessageTypes {
        M_TYPE_MASK         = 0xff000000,
        M_ARG_MASK          = 0x00ffffff,

        M_READ_REGISTERS    = 0x01000000,  // [] -> [r0-r9, FP, SP, PC, CPSR]
        M_WRITE_REGISTERS   = 0x02000000,  // [r0-r9, FP, SP, PC, CPSR] -> []
        M_READ_RAM          = 0x03000000,  // arg=address, [byteCount] -> [bytes]
        M_WRITE_RAM         = 0x04000000,  // arg=address, [bytes] -> []
    };
};


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

class SvmDebug {
public:
    SvmDebug();    // Do not implement

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

    static void fault(FaultCode code);

    static void setSymbolSourceELF(const FlashRange &elf);

#ifdef SIFTEO_SIMULATOR
    static std::string formatAddress(uint32_t address);
    static std::string formatAddress(void *address);
#endif
};


#endif // SVM_DEBUG_H
