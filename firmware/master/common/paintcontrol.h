/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PAINTCONTROL_H
#define _PAINTCONTROL_H

#include <sifteo/abi.h>
#include "systime.h"
#include "machine.h"
#include "vram.h"

class CubeSlot;


/**
 * Paint controller for one cube. This manages frame rate control, frame
 * triggering, and tracking rendering-finished state.
 */

class PaintControl {
 public:

    void waitForPaint();
    void waitForFinish(CubeSlot *cube);
    void triggerPaint(CubeSlot *cube, SysTime::Ticks timestamp);

    // Called in ISR context
    void ackFrames(CubeSlot *cube, int32_t count);
    void vramFlushed(CubeSlot *cube);

 private:
    SysTime::Ticks paintTimestamp;
    int32_t pendingFrames;

    uint8_t getFlags(_SYSVideoBuffer *vbuf) {
        return VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM, flags));
    }

    void setFlags(_SYSVideoBuffer *vbuf, uint8_t flags, uint32_t lockFlags) {
        VRAM::pokeb(*vbuf, offsetof(_SYSVideoRAM, flags), flags, lockFlags);
        VRAM::unlock(*vbuf);
    }
};


#endif
