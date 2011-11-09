/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "accel.h"
#include "runtime.h"
#include <stdlib.h>

//using namespace Sifteo;

AccelState AccelState::instances[_SYS_NUM_CUBE_SLOTS];

AccelState::AccelState() {
  uint8_t i;

  for (i = 0; i < NUM_AXES; i++) {
    dataSum[i] = 0;
  }

  currentDataIdx = 0;
}

/**
This will record samples and then look for tilts/shakes
*/
void AccelState::update(int8_t x, int8_t y) {
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


bool AccelState::updateTiltState() {
	int8_t curTiltState;
	bool tiltChanged = false;

	curTiltState = calculateTiltState(0);
	if (tiltState.x != curTiltState) {
		tiltState.x = curTiltState;
		tiltChanged = true;
	}

	curTiltState = calculateTiltState(1);
	if (tiltState.y != curTiltState) {
		tiltState.y = curTiltState;
		tiltChanged = true;
	}


	//DEBUG_LOG(("Updating tilt: x = %d, y = %d\n", tiltState.x, tiltState.y));
  
	if (tiltChanged) {
		Event::setPending(EventBits::TILT, id());
		return true;
	}

	return false;
}

int8_t AccelState::calculateTiltState(uint8_t axis) {
	int8_t tiltVal = GetMean(dataSum[axis]);

	if (tiltVal > (DEFAULT_RESTVAL + TILT_THRESHOLD))
		return _SYS_TILT_POSITIVE;
		
	if (tiltVal < (DEFAULT_RESTVAL - TILT_THRESHOLD))
		return _SYS_TILT_NEGATIVE;

	//includes hysteresis.  If you are in a non-neutral state, do not leave it unless
	//you are well past the threshold.  (prevents spiky behavior)
	int8_t curTiltState = axis == 0 ? tiltState.x : tiltState.y;

	switch (curTiltState) {
		case _SYS_TILT_POSITIVE:
			return (tiltVal < (DEFAULT_RESTVAL + TILT_THRESHOLD - TILT_HYSTERESIS)) ? _SYS_TILT_NEUTRAL : _SYS_TILT_POSITIVE;
		case _SYS_TILT_NEGATIVE:
			return (tiltVal > (DEFAULT_RESTVAL - TILT_THRESHOLD + TILT_HYSTERESIS)) ? _SYS_TILT_NEUTRAL : _SYS_TILT_NEGATIVE;
		default:
			return _SYS_TILT_NEUTRAL;
	}
}

void AccelState::updateShakeState() {
	_SYS_ShakeState newGlobalShakeState = NOT_SHAKING;
	uint8_t i;
 
	for (i = 0; i < NUM_AXES; i++) {
		_SYS_ShakeState ss;
		ss = calculateShakeState(i);
		//DEBUG_LOG(("axis %d, shaking = %d\n", i, ss));
		if (ss == SHAKING)
			newGlobalShakeState = SHAKING;
	}
  
	if (shakeState != newGlobalShakeState) {
		shakeState = newGlobalShakeState;
		//DEBUG_LOG(("shake state changing to %d\n", shakeState));
		Event::setPending(EventBits::SHAKE, id());
	}
}

_SYS_ShakeState AccelState::calculateShakeState(uint8_t axis) {
	uint8_t i;
	uint16_t variance = 0; //this sum will never exceed 16 bits over 32 samples
	const int16_t mean = GetMean(dataSum[axis]);
	/*
	THIS ALGO IS SIMPLE -- USING ABS() INSTEAD OF SQUARES IS NOT TOTALLY CORRECT 
	FOR VARIANCE, BUT IT IS PRETTY MUCH RIGHT, AND DEFINITELY TIME AND SPACE EFFICIENT...
   
	WHAT WE ARE CALCULATING IS ACTUALLY THE 'MEAN DEVIATION' OR 'ABSOLUTE MEAN DEVIATION', 
	WHICH, LIKE VARIANCE, IS A GOOD MEASURE OF DEVIATION.
	*/
	for (i = 0; i < NUM_SAMPLES; i++)
		variance += abs(data[axis][i] - mean); // add the difference between each data item and the mean
    
	//DEBUG_LOG(("Calculating shake.  Variance = %d\n", variance));

	if (variance > SHAKE_THRESHOLD)
		return SHAKING;
	//includes hysteresis.  If you are SHAKING, do not leave this state unless
	//you are well past the threshold.  (prevents spiky behavior)
	if (shakeState == SHAKING)
		return (variance < (SHAKE_THRESHOLD - SHAKE_HYSTERESIS)) ? NOT_SHAKING : SHAKING;
	else
		return NOT_SHAKING;
}