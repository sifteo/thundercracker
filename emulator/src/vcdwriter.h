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

/*
 * Object for writing Verilog Value Change Dump (VCD) files, a common interchange
 * format for digital logic simulation traces.
 *
 * For simplicity, we define signals in terms of existing memory variables.
 * Every defined signal is polled once per clock tick. (We never said this would be fast...)
 */

#ifndef _VCDWRITER_H
#define _VCDWRITER_H

#include <string>
#include <sstream>
#include <vector>
#include <stdio.h>

#include "macros.h"
#include "vtime.h"


class VCDWriter {
public:
    VCDWriter()
        : currentTick(-1) {}

    void enterScope(const std::string scope);
    void leaveScope();
    void setNamePrefix(const std::string prefix);
    void define(const std::string name, void *var, unsigned numBits=1, unsigned firstBit=0);

    void writeHeader(FILE *f);
    void writeTick(FILE *f, uint64_t clock);

private:
    struct SignalSource {
        SignalSource(void *var, unsigned numBits, unsigned firstBit)
            : value(-1), var(var), numBits(numBits), firstBit(firstBit) {};

        uint64_t sample()
        {
            uint64_t s;
            uint8_t total = numBits + firstBit;

            if (total <= 8)
                s = *(uint8_t*)var;
            else if (total <= 16)
                s = *(uint16_t*)var;
            else if (total <= 32)
                s = *(uint32_t*)var;
            else
                s = *(uint64_t*)var;

            return (s >> firstBit) & ((1 << numBits) - 1);
        }

        uint64_t value;

        void *var;
        uint8_t numBits;
        uint8_t firstBit;
    };

    std::vector<SignalSource> sources;
    std::vector<std::string> identifiers;
    std::string namePrefix;
    std::stringstream defs;
    uint64_t currentTick;
    
    std::string createIdentifier(unsigned id);
};

#endif
