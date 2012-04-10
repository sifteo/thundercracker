/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "accel.h"
#include "event.h"
#include <stdlib.h>

AccelState AccelState::instances[_SYS_NUM_CUBE_SLOTS];

AccelState::AccelState()
{
    uint8_t i;

    for (i = 0; i < NUM_AXES; i++) {
        dataSum[i] = 0;
    }

    currentDataIdx = 0;
}

/**
 * This will record samples and then look for tilts/shakes
 */
void AccelState::update(int8_t x, int8_t y)
{
    for (int i = 0; i < NUM_AXES; i++) {
        dataSum[i] -= data[i][currentDataIdx]; // remove old data from sum...
        data[i][currentDataIdx] = i == 0 ? x : y; // update data...
        dataSum[i] += data[i][currentDataIdx]; // add in new val to sum
        currentDataIdx++;
        currentDataIdx %= NUM_SAMPLES;
    }

    updateTiltState();
    updateShakeState();
}


bool AccelState::updateTiltState()
{
    int8_t curTiltState;
    bool tiltChanged = false;

    curTiltState = calculateTiltState(0);
    if (tiltX != curTiltState) {
        tiltX = curTiltState;
        tiltChanged = true;
    }

    curTiltState = calculateTiltState(1);
    if (tiltY != curTiltState) {
        tiltY = curTiltState;
        tiltChanged = true;
    }

    if (tiltChanged) {
        Event::setPending(_SYS_CUBE_TILT, id());
        return true;
    }

    return false;
}

int AccelState::calculateTiltState(uint8_t axis)
{
    int8_t tiltVal = GetMean(dataSum[axis]);

    if (tiltVal > (DEFAULT_RESTVAL + TILT_THRESHOLD))
        return 1;
        
    if (tiltVal < (DEFAULT_RESTVAL - TILT_THRESHOLD))
        return -1;

    // Includes hysteresis.  If you are in a non-neutral state, do not leave it unless
    // you are well past the threshold.  (prevents spiky behavior)
    int8_t curTiltState = axis == 0 ? tiltX : tiltY;

    switch (curTiltState) {
        case 1:
            return (tiltVal < (DEFAULT_RESTVAL + TILT_THRESHOLD - TILT_HYSTERESIS)) ? 0 : 1;
        case -1:
            return (tiltVal > (DEFAULT_RESTVAL - TILT_THRESHOLD + TILT_HYSTERESIS)) ? 0 : -1;
        default:
            return 0;
    }
}

void AccelState::updateShakeState()
{
    bool newGlobalShakeState = false;
    uint8_t i;
 
    for (i = 0; i < NUM_AXES; i++) {
        if (calculateShakeState(i))
            newGlobalShakeState = true;
    }
  
    if (shakeState != newGlobalShakeState) {
        shakeState = newGlobalShakeState;
        Event::setPending(_SYS_CUBE_SHAKE, id());
    }
}

bool AccelState::calculateShakeState(uint8_t axis)
{
    uint8_t i;
    uint16_t variance = 0; //this sum will never exceed 16 bits over 32 samples
    const int16_t mean = GetMean(dataSum[axis]);
 
    /*
     * This algo is simple -- using abs() instead of squares is not
     * totally correct for variance, but it is pretty much right, and
     * definitely time and space efficient...
     *
     * what we are calculating is actually the 'mean deviation' or
     * 'absolute mean deviation', which, like variance, is a good
     * measure of deviation.
     */
    for (i = 0; i < NUM_SAMPLES; i++) {
        // Add the difference between each data item and the mean
        variance += abs(data[axis][i] - mean);
    }
        
    if (variance > SHAKE_THRESHOLD)
        return true;

    // Includes hysteresis.  If you are SHAKING, do not leave this state unless
    // you are well past the threshold.  (prevents spiky behavior)

    if (shakeState == true)
        return variance >= (SHAKE_THRESHOLD - SHAKE_HYSTERESIS);
    return false;
}