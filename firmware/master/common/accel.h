/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ACCEL_H
#define _ACCEL_H

#include <sifteo/abi.h>
#include "macros.h"

/**
 *Accelerometer state for cubes
 *tilt/shake code from Gen 1
 */

class AccelState {
 public:
    static const int NUM_AXES = 2;

    //bunch of constants from gen 1..  maybe they need to be revisited
    static const int TILT_THRESHOLD   = 14;//12;  // measured empirically (gen 1)
    static const int DEFAULT_RESTVAL  = 0;
    static const int TILT_HYSTERESIS  = 4;//5;   // measured empirically (gen 1)

    static const int NUM_SAMPLES      = 16; //how many samples we keep in the data buffer
    static const int MEAN_SHIFT_BY    = 4; //if NUM_SAMPLES is 16, we can bit shift the sum by 4 to get the mean

    static const int SHAKE_THRESHOLD  = (NUM_SAMPLES * 38);  // Gen 1 @ 100HZ would be 45
    static const int SHAKE_HYSTERESIS = (NUM_SAMPLES * 21);  // Gen 1 @ 100HZ would be 25

    static AccelState instances[_SYS_NUM_CUBE_SLOTS];

    static AccelState &getInstance(_SYSCubeID id) {
        ASSERT(id < _SYS_NUM_CUBE_SLOTS);
        return instances[id];
    }

    _SYSCubeID id() const {
        _SYSCubeID i = this - &instances[0];
        ASSERT(i < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(arraysize(instances) == _SYS_NUM_CUBE_SLOTS);
        return i;
    }

    AccelState();

    inline int16_t GetMean(int16_t sum)  { return (sum >> MEAN_SHIFT_BY); }

    _SYSByte4 getTiltState() {
        _SYSByte4 tilt;
        tilt.value = 0;
        tilt.x = tiltX;
        tilt.y = tiltY;
        return tilt;
    }

    uint32_t getShakeState() {
        return shakeState;
    }

    void update(int8_t x, int8_t y);

    //these functions are all based on gen 1's implementations
    bool updateTiltState(void);
    int calculateTiltState(uint8_t axis);
    void updateShakeState();
    bool calculateShakeState(uint8_t axis);

 private:
    int8_t data[NUM_AXES][NUM_SAMPLES];
    int16_t dataSum[NUM_AXES];
    uint8_t currentDataIdx;
    uint8_t tiltX, tiltY;
    uint8_t shakeState;
};


#endif
