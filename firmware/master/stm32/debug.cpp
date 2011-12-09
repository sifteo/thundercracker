/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "hardware.h"

#ifdef DEBUG

uint8_t Debug::buffer[DEBUG_BUFFER_SIZE];
uint32_t Debug::bufferLen;


void Debug::dcc(uint32_t dccData)
{
    /*
     * Raw debug data for OpenOCD. Blocks indefinitely if OpenOCD
     * is not attached!
     */
       
     int bytes = 4;
     const uint32_t busy = 1;

     do {
         while (NVIC_SIFTEO.DCRDR_l & busy);
         NVIC_SIFTEO.DCRDR_l = ((dccData & 0xFF) << 8) | busy;
         dccData >>= 8;
     } while (--bytes);
}

void Debug::log(const char *str)
{
    /*
     * Low-level JTAG debug messages. Blocks indefinitely if
     * OpenOCD is not attached! To get these messages, run
     * "monitor target_request debugmsgs enable" in GDB.
     */

    const uint32_t command = 0x01;
    int len = 0;

    for (const char *p = str; *p && len < 0xFFFF; p++, len++);
    
    dcc(command | (len << 16));
    
    while (len > 0) {
        uint32_t data = ( str[0] 
                          | ((len > 1) ? (str[1] << 8) : 0)
                          | ((len > 2) ? (str[2] << 16) : 0)
                          | ((len > 3) ? (str[3] << 24) : 0) );
        dcc(data);
        len -= 4;
        str += 4;
    }
}

void Debug::logToBuffer(void const *data, uint32_t size)
{
    /*
     * Log some raw data to the buffer. It will get dumped en-masse to
     * OpenOCD only when the buffer is too full to accept the next
     * message, or when it's explicitly flushed.
     */

    if (size + bufferLen >= DEBUG_BUFFER_SIZE)
        dumpBuffer();

    buffer[bufferLen++] = size;
    memcpy(buffer + bufferLen, data, size);
    bufferLen += size;
}        

void Debug::dumpBuffer()
{
    static char line[DEBUG_BUFFER_MSG_SIZE * 4];
    uint8_t *p = buffer;
    uint8_t *end = buffer + bufferLen;

    while (p < end) {
        char *linePtr = line;
        uint8_t msgLen = *(p++);
        linePtr += sprintf(linePtr, "[%2d] ", msgLen);
        
        while (msgLen && p < end) {
            linePtr += sprintf(linePtr, "%02x", *(p++));
            msgLen--;
        }

        log(line);
    }
}

#endif  // DEBUG
