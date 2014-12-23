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

#ifndef _STM32_DEBUG_H
#define _STM32_DEBUG_H

#ifdef DEBUG
#define DEBUG_STUB
#else
#define DEBUG_STUB  {}
#endif

#define DEBUG_BUFFER_SIZE       16384
#define DEBUG_BUFFER_MSG_SIZE   32

#include <stdint.h>

class Debug {
 public:
    static void dcc(uint32_t dccData) DEBUG_STUB;
    static void log(const char *str) DEBUG_STUB;

    static void logToBuffer(void const *data, uint32_t size) DEBUG_STUB;
    static void dumpBuffer() DEBUG_STUB;

 private:
    static uint8_t buffer[DEBUG_BUFFER_SIZE];
    static uint32_t bufferLen;
};

#endif  // _STM32_DEBUG_H
