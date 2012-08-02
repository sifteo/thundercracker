/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "motion.h"


_SYSByte4 MotionUtil::captureAccelState(const RF_ACKType &ack)
{
    /*
     * All bytes in protocol happen to be inverted
     * relative to the SDK's coordinate system.
     */

    _SYSByte4 state;
    state.x = -ack.accel[0];
    state.y = -ack.accel[1];
    state.z = -ack.accel[2];
    state.w = 0;
    return state;
}
