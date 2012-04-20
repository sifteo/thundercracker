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

    // Start doing a _SYS_finish()
    void beginFinish(CubeSlot *cube);

    // Inner loop for _SYS_finish(). Returns 'true' when finished.
    bool pollForFinish(CubeSlot *cube, SysTime::Ticks now);

    // Two halves of _SYS_paint()
    void waitForPaint(CubeSlot *cube);
    void triggerPaint(CubeSlot *cube, SysTime::Ticks now);

    // Called in ISR context
    void ackFrames(CubeSlot *cube, int32_t count);
    void vramFlushed(CubeSlot *cube);

 private:
    SysTime::Ticks paintTimestamp;      // Last user call to _SYS_paint()
    SysTime::Ticks asyncTimestamp;      // TOGGLE, TRIGGER_ON_FLUSH, exiting CONTINUOUS mode
    int32_t pendingFrames;

    uint8_t getFlags(_SYSVideoBuffer *vbuf) {
        return VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM, flags));
    }

    void setFlags(_SYSVideoBuffer *vbuf, uint8_t flags) {
        // Set no flags on lock
        VRAM::pokeb(*vbuf, offsetof(_SYSVideoRAM, flags), flags, 0);
        VRAM::unlock(*vbuf);
    }

    static bool allowContinuous(CubeSlot *cube);
    void enterContinuous(CubeSlot *cube, _SYSVideoBuffer *vbuf, uint8_t &flags);
    void exitContinuous(CubeSlot *cube, _SYSVideoBuffer *vbuf, uint8_t &flags, SysTime::Ticks timestamp);
    bool isContinuous(_SYSVideoBuffer *vbuf);
    void setToggle(CubeSlot *cube, _SYSVideoBuffer *vbuf, 
        uint8_t &flags, SysTime::Ticks timestamp);
    void makeSynchronous(CubeSlot *cube, _SYSVideoBuffer *vbuf);
    bool canMakeSynchronous(_SYSVideoBuffer *vbuf, SysTime::Ticks timestamp);
};


#endif
