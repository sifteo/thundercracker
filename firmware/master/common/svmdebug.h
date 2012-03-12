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

using namespace Svm;


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


class SvmDebug {
public:
    SvmDebug();    // Do not implement

    static const unsigned LOG_BUFFER_WORDS = 7;
    static const unsigned LOG_BUFFER_BYTES = LOG_BUFFER_WORDS * sizeof(uint32_t);

    static uint32_t *logReserve(SvmLogTag tag);
    static void logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes);

    static void fault(FaultCode code);

    static void setSymbolSourceELF(const FlashRange &elf);
};


#endif // SVM_DEBUG_H
