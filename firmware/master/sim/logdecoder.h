/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef LOG_DECODER_H
#define LOG_DECODER_H

#include "elfdebuginfo.h"
#include "svmdebug.h"

class LogDecoder {
public:
    LogDecoder();   // Do not implement

    // Decode a log entry with the specified tag and data buffer.
    // Returns the actual number of bytes consumed from 'buffer'.
    static size_t decode(ELFDebugInfo &DI, SvmLogTag tag, uint32_t *buffer);

private:
    static void formatLog(char *out, size_t outSize,
        char *fmt, uint32_t *args, size_t argCount);
};

#endif  // LOG_DECODER_H
