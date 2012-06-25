/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _VOLUME_H
#define _VOLUME_H

#include <stdint.h>

namespace Volume
{
    enum CalibrationState {
        CalibrationLow,
        CalibrationHigh
    };

    void init();

    int systemVolume();  // current system volume
    int calibrate(CalibrationState state);
}

#endif // _VOLUME_H
