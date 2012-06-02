/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _WAVEFILE_H
#define _WAVEFILE_H

#include <stdio.h>
#include <stdint.h>


class WaveWriter
{
public:
    WaveWriter();

    bool open(const char *filename, unsigned sampleRate);
    void close();

    // Assumes 16-bit signed mono. No-op if file is closed.
    void write(const int16_t *samples, unsigned count);

    bool isOpen() const {
        return file != 0;
    }

    unsigned getSampleCount() const {
        return sampleCount;
    }

private:
    FILE *file;
    unsigned sampleCount;

    void write32(uint32_t word);
};


#endif
