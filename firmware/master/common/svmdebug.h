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

    inline uint32_t getStringOffset() {
        return tag & 0xffffff;
    }
    
    inline uint32_t getArity() {
        return (tag >> 24) & 7;
    }
    
private:
    uint32_t tag;
};


class SvmDebug {
public:
    SvmDebug();    // Do not implement

    static uint32_t *logReserve(SvmLogTag tag);
    static void logCommit(SvmLogTag tag, uint32_t *buffer);

    static void fault(FaultCode code);

    static void setSymbolSourceELF(const FlashRange &elf);
};


#endif // SVM_DEBUG_H
