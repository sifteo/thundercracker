/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Debugger bridge for SVM. This is the connection between debug-related
 * stubs in the SVM runtime, and a host system.
 */

#ifndef SVM_DEBUGPIPE_H
#define SVM_DEBUGPIPE_H

#include "macros.h"
#include "svm.h"
#include "elfprogram.h"
#include "flash_blockcache.h"
#include "flash_map.h"

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

    // Called when a new program is first starting
    static void init();

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

    static void setSymbolSource(const Elf::Program &program);

#ifdef SIFTEO_SIMULATOR
    static std::string formatAddress(uint32_t address);
    static std::string formatAddress(void *address);
#endif
};


#endif // SVM_DEBUGPIPE_H
