/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
