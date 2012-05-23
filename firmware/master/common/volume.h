#ifndef VOLUME_H
#define VOLUME_H

#include <stdint.h>

namespace Volume
{
    enum CalibrationState {
        CalibrationLow,
        CalibrationHigh
    };

    void init();
    uint16_t systemVolume();  // current system volume
    uint16_t calibrate(CalibrationState state);
}

#endif // VOLUME_H
