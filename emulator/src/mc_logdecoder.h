/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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

#ifndef LOG_DECODER_H
#define LOG_DECODER_H

#include "mc_elfdebuginfo.h"
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
    size_t decode(ELFDebugInfo &DI, SvmLogTag tag, uint32_t *buffer);

private:
    void formatLog(ELFDebugInfo &DI, char *out, size_t outSize,
        char *fmt, uint32_t *args, size_t argCount);

    void writeLog(const char *str);
    void runScript();

    unsigned scriptType;
    std::string scriptBuffer;
    std::map<unsigned, ScriptHandler> handlers;
};

#endif  // LOG_DECODER_H
