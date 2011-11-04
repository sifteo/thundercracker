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


bool AccelState::updateTiltState() {
  int8_t curTiltState;
  bool tiltChanged = false;

  curTiltState = calculateTiltState(0);
    if (tiltState.x != curTiltState) {
      tiltState.x != curTiltState;
      tiltChanged = true;
    }

	curTiltState = calculateTiltState(1);
    if (tiltState.y != curTiltState) {
      tiltState.y != curTiltState;
      tiltChanged = true;
    }

  
  if (tiltChanged) {
	Event::setPending(EventBits::TILT, id());
    return true;
  }
  return false;
}

int8_t AccelState::calculateTiltState(int8_t axis) {
  int8_t tiltVal = GetMean(dataSum[axis]);

  if (tiltVal > (DEFAULT_RESTVAL + TILT_THRESHOLD))
    return _SYS_TILT_NEGATIVE;

  if (tiltVal < (DEFAULT_RESTVAL - TILT_THRESHOLD))
    return _SYS_TILT_POSITIVE;

  //includes hysteresis.  If you are in a non-neutral state, do not leave it unless
  //you are well past the threshold.  (prevents spiky behavior)
  int8_t curTiltState = axis == 0 ? tiltState.x : tiltState.y;

  switch (curTiltState) {
    case _SYS_TILT_NEGATIVE:
      return (tiltVal < (DEFAULT_RESTVAL + TILT_THRESHOLD - TILT_HYSTERESIS)) ? _SYS_TILT_NEUTRAL : _SYS_TILT_NEGATIVE;
    case _SYS_TILT_POSITIVE:
      return (tiltVal > (DEFAULT_RESTVAL - TILT_THRESHOLD + TILT_HYSTERESIS)) ? _SYS_TILT_NEUTRAL : _SYS_TILT_POSITIVE;
    default:
      return _SYS_TILT_NEUTRAL;
  }
}

void AccelState::updateShakeState() {
	e_ShakeState newGlobalShakeState = NOT_SHAKING;
  uint8_t i;
  
  for (i = 0; i < NUM_AXES; i++) {
    e_ShakeState ss;
    ss = calculateShakeState(i);
    if (ss == SHAKING && ss != shakeState)
      newGlobalShakeState = SHAKING;
  }
  
  if (shakeState != newGlobalShakeState) {
	  shakeState = newGlobalShakeState;
	  Event::setPending(EventBits::SHAKE, id());
  }
}

e_ShakeState AccelState::calculateShakeState(uint8_t axis) {
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
    
  if (variance > SHAKE_THRESHOLD)
    return SHAKING;
  //includes hysteresis.  If you are SHAKING, do not leave this state unless
  //you are well past the threshold.  (prevents spiky behavior)
  if (shakeState == SHAKING)
    return (variance < (SHAKE_THRESHOLD - SHAKE_HYSTERESIS)) ? NOT_SHAKING : SHAKING;
  else
    return NOT_SHAKING;
}