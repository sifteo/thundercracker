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

#include "macros.h"
#include "tracer.h"
#include "vtime.h"

bool Tracer::enabled;
Tracer *Tracer::instance;


void Tracer::setEnabled(bool b)
{
    if (b) {
        instance = this;
        
        if (!textTraceFile) {
            textTraceFile = fopen("trace.txt", "w");
        }

        if (!vcdTraceFile) {
            vcdTraceFile = fopen("trace.vcd", "w");
            if (vcdTraceFile)
                vcd.writeHeader(vcdTraceFile);
        }
            
        enabled = textTraceFile && vcdTraceFile;
        if (!enabled)
            fprintf(stderr, "Tracer: Error opening output file(s)!\n");

    } else {
        enabled = false;

        fflush(textTraceFile);
        fflush(vcdTraceFile);
    }
}

void Tracer::close()
{
    setEnabled(false);
    
    if (textTraceFile) {
        fclose(textTraceFile);
        textTraceFile = NULL;
    }

    if (vcdTraceFile) {
        fclose(vcdTraceFile);
        vcdTraceFile = NULL;
    }
}

void Tracer::logWork(const Cube::CPU::em8051 *cpu)
{
    fprintf(textTraceFile, "[%02d t=%"PRIu64"] ", cpu->id, getLocalClock(*cpu->vtime));
}

void Tracer::logWork(const Cube::CPU::em8051 *cpu, const char *fmt, va_list ap)
{
    logWork(cpu);
    vfprintf(textTraceFile, fmt, ap);
    fprintf(textTraceFile, "\n");
}

void Tracer::logHexWork(const Cube::CPU::em8051 *cpu, const char *msg, size_t len, void *data)
{
    logWork(cpu);

    fprintf(textTraceFile, "%s [%u]", msg, (unsigned)len);

    uint8_t *bytes = (uint8_t*) data;
    while (len) {
        fprintf(textTraceFile, " %02x", *bytes);
        len--;
        bytes++;
    }
    
    fprintf(textTraceFile, "\n");
}
