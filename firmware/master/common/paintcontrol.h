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
 * The tiny VRAMFlags class provides a convenient way to batch changes
 * to the 'flags' byte in VRAM. Changes are always applied via atomic XOR.
 */

struct VRAMFlags {
    uint8_t vf;
    uint8_t vfPrev;

    VRAMFlags(_SYSVideoBuffer *vbuf) {
        vf = vfPrev = VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM, flags));
    }

    bool test(uint8_t flags) const { return (vf & flags) == flags; }
    void set(uint8_t flags) { vf |= flags; }
    void clear(uint8_t flags) { vf &= ~flags; }
    void toggle(uint8_t flags) { vf ^= flags; }

    void setTo(uint8_t flags, bool b) {
        if (b)
            set(flags);
        else
            clear(flags);
    }

    bool apply(_SYSVideoBuffer *vbuf);
};


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
    void waitForPaint(CubeSlot *cube, uint32_t excludedTasks);
    void triggerPaint(CubeSlot *cube, SysTime::Ticks now);

    // Called in ISR context
    void ackFrames(CubeSlot *cube, int32_t count);
    bool vramFlushed(CubeSlot *cube);

 private:
    SysTime::Ticks paintTimestamp;      // Last user call to _SYS_paint()
    SysTime::Ticks asyncTimestamp;      // TOGGLE, TRIGGER_ON_FLUSH, entering CONTINUOUS mode
    int32_t pendingFrames;

    static bool allowContinuous(CubeSlot *cube);
    void enterContinuous(CubeSlot *cube, _SYSVideoBuffer *vbuf,
        VRAMFlags &flags, SysTime::Ticks timestamp);
    void exitContinuous(CubeSlot *cube, _SYSVideoBuffer *vbuf, VRAMFlags &flags);
    void setToggle(CubeSlot *cube, _SYSVideoBuffer *vbuf, 
        VRAMFlags &flags, SysTime::Ticks timestamp);
    void makeSynchronous(CubeSlot *cube, _SYSVideoBuffer *vbuf);
    bool canMakeSynchronous(CubeSlot *cube, _SYSVideoBuffer *vbuf,
        VRAMFlags &flags, SysTime::Ticks timestamp);
};


#endif
