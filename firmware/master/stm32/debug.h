/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
