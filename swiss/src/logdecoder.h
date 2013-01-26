/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef LOG_DECODER_H
#define LOG_DECODER_H

#include "elfdebuginfo.h"
#include "svmdebugpipe.h"

#include <string>
#include <map>

class LogDecoder {
public:
    struct ScriptHandler {
        void (*func)(const char *str, void *context);
        void *context;
    };

    // Reset the internal state of the decoder
    void init();

    // Add a handler for a new kind of script
    void setScriptHandler(unsigned type, ScriptHandler handler);

    // Decode a log entry with the specified tag and data buffer.
    // Returns the actual number of bytes consumed from 'buffer'.
    size_t decode(ELFDebugInfo &DI, SvmLogTag tag, const uint32_t *buffer);

private:
    void formatLog(ELFDebugInfo &DI, char *out, size_t outSize,
        char *fmt, const uint32_t *args, size_t argCount);

    void writeLog(const char *str);
    void runScript();

    unsigned scriptType;
    std::string scriptBuffer;
    std::map<unsigned, ScriptHandler> handlers;
};

#endif  // LOG_DECODER_H
