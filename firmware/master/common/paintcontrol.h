/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PAINTCONTROL_H
#define _PAINTCONTROL_H

#include <sifteo/abi.h>
#include "systime.h"
#include "machine.h"

class CubeSlot;


/**
 * Paint controller for one cube. This manages frame rate control, frame
 * triggering, and tracking rendering-finished state.
 */

class PaintControl {
 public:

    void waitForPaint();
    void waitForFinish(_SYSVideoBuffer *vbuf);
    void triggerPaint(CubeSlot *cube, SysTime::Ticks timestamp);

    void ackFrames(uint8_t count) {
         Atomic::Add(pendingFrames, -(int32_t)count);
    }

 private:
    SysTime::Ticks paintTimestamp;
    int32_t pendingFrames;
};


#endif
